/**\file
 * \brief RhrClient plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

#include <config/CoviseConfig.h>

#include <cover/coVRConfig.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRMSController.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coVRPluginList.h>
#include <cover/VRViewer.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRLighting.h>
#include <cover/coVRAnimationManager.h>

#include <cover/mui/Tab.h>
#include <cover/mui/ToggleButton.h>
#include <cover/mui/LabelElement.h>

#include <PluginUtil/PluginMessageTypes.h>

#include <OpenVRUI/coSubMenuItem.h>
#include <OpenVRUI/coRowMenu.h>
#include <OpenVRUI/coCheckboxMenuItem.h>
#include <OpenVRUI/coPotiMenuItem.h>

#include <osgGA/GUIEventAdapter>
#include <osg/Material>
#include <osg/TexEnv>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/TextureRectangle>
#include <osgViewer/Renderer>

#include <osg/io_utils>

#include "RhrClient.h"

#include <rhr/rfbext.h>


#include <tbb/parallel_for.h>
#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>
#include <memory>

#include <thread>
#include <mutex>

#include <boost/lexical_cast.hpp>

#ifdef __linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#endif

//#define CONNDEBUG

#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

using message::RemoteRenderMessage;
#include <core/tcpmessage.h>

#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>

struct TjDecomp {

   TjDecomp()
      : handle(tjInitDecompress())
      {}

   ~TjDecomp() {
      tjDestroy(handle);
   }

   tjhandle handle;
};

typedef tbb::enumerable_thread_specific<TjDecomp> TjContext;
static TjContext tjContexts;
#endif

typedef std::lock_guard<std::recursive_mutex> lock_guard;

RhrClient *RhrClient::plugin = NULL;

class RemoteConnection {
    public:
    RhrClient *plugin = nullptr;
    //! store results of server bounding sphere query
    bool m_boundsUpdated = false;
    osg::ref_ptr<osg::Node> boundsNode;

    std::string m_host;
    unsigned short m_port;
    asio::ip::tcp::socket m_sock;
    std::recursive_mutex *m_mutex = nullptr;
    std::thread *m_thread = nullptr;
    bool haveMessage = false;
    bool m_running = false;
    bool m_interrupt = false;
    bool m_isMaster = false;
    std::deque<TileMessage> m_receivedTiles;

    RemoteConnection() = delete;
    RemoteConnection(const RemoteConnection& other) = delete;
    RemoteConnection(RhrClient *plugin, std::string host, unsigned short port, bool isMaster)
        : plugin(plugin)
        , m_host(host)
        , m_port(port)
        , m_sock(plugin->m_io)
    {
        boundsNode = new osg::Node;
        m_mutex = new std::recursive_mutex;
        if (isMaster) {
            m_running = true;
            m_thread = new std::thread(std::ref(*this));
        }
        //m_endpoint
    }

    ~RemoteConnection() {

        if (!m_thread) {
            assert(!m_sock.is_open());
        } else {
            stopThread();
            m_thread->join();
            delete m_thread;
        }
        delete m_mutex;
    }

    void stopThread() {

        {
            lock_guard locker(*m_mutex);
            m_interrupt = true;
        }
        {
            lock_guard locker(*m_mutex);
            if (m_sock.is_open()) {
                boost::system::error_code ec;
                m_sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            }
        }

        for (;;) {
            {
                lock_guard locker(*m_mutex);
                if (!m_running)
                    break;
            }
            usleep(10000);
        }

        lock_guard locker(*m_mutex);
        assert(!m_running);

        std::cerr << "disconnected from server" << std::endl;
    }

    void operator()() {
#ifdef __linux
        pthread_setname_np(pthread_self(), "RhrClient");
#endif
        {
            lock_guard locker(*m_mutex);
            assert(m_running);
        }

        asio::ip::tcp::resolver resolver(plugin->m_io);
        asio::ip::tcp::resolver::query query(m_host, boost::lexical_cast<std::string>(m_port));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::system::error_code ec;
        asio::connect(m_sock, endpoint_iterator, ec);
        if (ec) {
            std::cerr << "RhrClient: could not establish connection to " << m_host << ":" << m_port << std::endl;
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }

        for (;;) {
            if (!m_sock.is_open())
                break;

            message::Buffer buf;
            auto payload = std::make_shared<std::vector<char>>();
            bool received = false;
            bool ok = message::recv(m_sock, buf, received, false, payload.get());
            if (!ok) {
                break;
            }
            {
                lock_guard locker(*m_mutex);
                if (m_interrupt) {
                    break;
                }
            }
            if (!received) {
                usleep(10000);
                continue;
            }

            if (buf.type() != message::REMOTERENDERING) {
                std::cerr << "invalid message type received" << std::endl;
                break;
            }

            {
                lock_guard locker(*m_mutex);
                haveMessage = true;
                auto &msg = buf.as<RemoteRenderMessage>();
                auto &rhr = msg.rhr();
                switch (rhr.type) {
                case rfbBounds: {
                    handleBounds(msg, static_cast<const boundsMsg &>(rhr));
                    break;
                }
                case rfbTile: {
                    handleTile(msg, static_cast<const tileMsg &>(rhr), payload);
                    break;
                }
                }
            }

            lock_guard locker(*m_mutex);
            if (m_interrupt) {
                break;
            }
        }
        lock_guard locker(*m_mutex);
        m_running = false;
    }


    //! handle RFB bounds message
    bool handleBounds(const RemoteRenderMessage &msg, const boundsMsg &bound) {

        std::cerr << "receiving bounds msg..." << std::endl;

        //std::cerr << "center: " << bound.center[0] << " " << bound.center[1] << " " << bound.center[2] << ", radius: " << bound.radius << std::endl;

        {
            lock_guard locker(*m_mutex);
            m_boundsUpdated = true;
            boundsNode->setInitialBound(osg::BoundingSphere(osg::Vec3(bound.center[0], bound.center[1], bound.center[2]), bound.radius));
        }

        osg::BoundingSphere bs = boundsNode->getBound();
        std::cerr << "server bounds: " << bs.center() << ", " << bs.radius() << std::endl;

        return true;
    }

    bool handleTile(const RemoteRenderMessage &msg, const tileMsg &tile, std::shared_ptr<std::vector<char>> payload) {

#ifdef CONNDEBUG
        int flags = tile.flags;
        bool first = flags&rfbTileFirst;
        bool last = flags&rfbTileLast;
        std::cerr << "rfbTileMessage: req: " << tile.requestNumber << ", first: " << first << ", last: " << last << std::endl;
#endif

        if (tile.size==0 && tile.width==0 && tile.height==0 && tile.flags == 0) {
            std::cerr << "received initial tile - ignoring" << std::endl;
            return true;
        }

        auto t = std::make_shared<tileMsg>(tile);
        m_receivedTiles.push_back(TileMessage(t, payload));

#if 0
        std::lock_guard<std::mutex> locker(plugin->m_pluginMutex);
        plugin->m_receivedTiles.push_back(TileMessage(t, payload));
        if (tile.flags & rfbTileLast)
            plugin->m_lastTileAt = plugin->m_receivedTiles.size();
#endif

        return true;
    }

    bool receive() {

        if (!m_sock.is_open())
            return false;

        message::Buffer buf;
        auto payload = std::make_shared<std::vector<char>>();
        bool received = false;
        bool ok = message::recv(m_sock, buf, received, false, payload.get());
        if (!ok) {
            return false;
        }
        if (!received) {
            return false;
        }

        if (buf.type() != message::REMOTERENDERING) {
            std::cerr << "invalid message type received" << std::endl;
            return false;
        }

        lock_guard locker(*m_mutex);
        haveMessage = true;
        auto &msg = buf.as<RemoteRenderMessage>();
        auto &rhr = msg.rhr();
        switch (rhr.type) {
        case rfbBounds: {
            handleBounds(msg, static_cast<const boundsMsg &>(rhr));
            break;
        }
        case rfbTile: {
            handleTile(msg, static_cast<const tileMsg &>(rhr), payload);
            break;
        }
        }
        return true;
    }

    bool isRunning() const {
        lock_guard locker(*m_mutex);
        return m_running;
    }

    bool isConnecting() const {
        lock_guard locker(*m_mutex);
        return m_sock.is_open();
    }

    bool isConnected() const {
        lock_guard locker(*m_mutex);
        return m_sock.is_open();
    }

    bool messageReceived() {
        lock_guard locker(*m_mutex);
        bool ret = haveMessage;
        haveMessage = false;
        return ret;
    }

    bool boundsUpdated() {
        lock_guard locker(*m_mutex);
        bool ret = m_boundsUpdated;
        m_boundsUpdated = false;
        return ret;
    }

    bool send(const message::Message &msg, const std::vector<char> *payload=nullptr) {
        lock_guard locker(*m_mutex);
        if (m_sock.is_open())
            return message::send(m_sock, msg, payload);
        else
            return false;
    }

    bool requestBounds() {
        lock_guard locker(*m_mutex);
        m_boundsUpdated = false;

        boundsMsg msg;
        msg.type = rfbBounds;
        msg.sendreply = 1;
        RemoteRenderMessage rrm(msg);
        return send(rrm);
    }

    void lock() {
        m_mutex->lock();
    }

    void unlock() {
        m_mutex->unlock();
    }
};

static const std::string config("COVER.Plugin.RhrClient");
static std::string muiId(const std::string &id = "") {
   const std::string base = "plugins.vistle.RhrClient";
   if (id.empty())
      return base;
   return base + "." + id;
}

void RhrClient::fillMatricesMessage(matricesMsg &msg, int channel, int viewNum, bool second) {

   msg.type = rfbMatrices;
   msg.viewNum = m_channelBase + viewNum;
   msg.time = cover->frameTime();
   const osg::Matrix &t = cover->getXformMat();
   const osg::Matrix &s = cover->getObjectsScale()->getMatrix();
   osg::Matrix model = s * t;
   if (m_noModelUpdate)
       model = m_oldModelMatrix;
   else
       m_oldModelMatrix = model;

   const channelStruct &chan = coVRConfig::instance()->channels[channel];
   if (chan.PBONum >= 0) {
       const PBOStruct &pbo = coVRConfig::instance()->PBOs[chan.PBONum];
       msg.width = pbo.PBOsx;
       msg.height = pbo.PBOsy;
   } else {
       if (chan.viewportNum < 0)
           return;
       const viewportStruct &vp = coVRConfig::instance()->viewports[chan.viewportNum];
       if (vp.window < 0)
           return;
       const windowStruct &win = coVRConfig::instance()->windows[vp.window];
       msg.width = (vp.viewportXMax - vp.viewportXMin) * win.sx;
       msg.height = (vp.viewportYMax - vp.viewportYMin) * win.sy;
   }

   bool left = chan.stereoMode == osg::DisplaySettings::LEFT_EYE;
   if (second)
      left = true;
   const osg::Matrix &view = left ? chan.leftView : chan.rightView;
   const osg::Matrix &proj = left ? chan.leftProj : chan.rightProj;

#if 0
   std::cerr << "retrieving matrices for channel: " << channel << ", view: " << viewNum << ", " << msg.width << "x" << msg.height << ", second: " << second << ", left: " << left << std::endl;
   std::cerr << "  view mat: " << view << std::endl;
   std::cerr << "  proj mat: " << proj << std::endl;
#endif

   for (int i=0; i<16; ++i) {
      //TODO: swap to big-endian
      msg.model[i] = model.ptr()[i];
      msg.view[i] = view.ptr()[i];
      msg.proj[i] = proj.ptr()[i];
   }
}

//! send matrices to server
bool RhrClient::sendMatricesMessage(std::shared_ptr<RemoteConnection> remote, std::vector<matricesMsg> &messages, uint32_t requestNum) {
   
   if (!remote)
      return false;

   messages.back().last = 1;
   //std::cerr << "requesting " << messages.size() << " views" << std::endl;
   for (size_t i=0; i<messages.size(); ++i) {
      matricesMsg &msg = messages[i];
      msg.requestNumber = requestNum;
      RemoteRenderMessage rrm(msg, 0);
      if (!remote->send(rrm))
          return false;
   }
   return true;
}

//! send lighting parameters to server
bool RhrClient::sendLightsMessage(std::shared_ptr<RemoteConnection> remote) {

   if (!remote)
      return false;

   //std::cerr << "sending lights" << std::endl;
   lightsMsg msg;
   msg.type = rfbLights;

   for (size_t i=0; i<lightsMsg::NumLights; ++i) {

      const osg::Light *light = NULL;
      const auto &lightList = coVRLighting::instance()->lightList;
#if 0
      if (i == 0 && coVRLighting::instance()->headlight) {
         light = coVRLighting::instance()->headlight->getLight();
      } else if (i == 1 && coVRLighting::instance()->spotlight) {
         light = coVRLighting::instance()->spotlight->getLight();
      } else if (i == 2 && coVRLighting::instance()->light1) {
         light = coVRLighting::instance()->light1->getLight();
      } else if (i == 3 && coVRLighting::instance()->light2) {
         light = coVRLighting::instance()->light2->getLight();
      }
#else
      if (i < lightList.size())
          light = lightList[i].source->getLight();
#endif

#define COPY_VEC(d, dst, src) \
      for (int k=0; k<d; ++k) { \
         (dst)[k] = (src)[k]; \
      }

      if (light) {

         msg.lights[i].enabled = lightList[i].on;
         if (i == 0) {
             // headlight is always enabled for menu sub-graph
             msg.lights[i].enabled = coVRLighting::instance()->headlightState;
         }
         COPY_VEC(4, msg.lights[i].position, light->getPosition());
         COPY_VEC(4, msg.lights[i].ambient, light->getAmbient());
         COPY_VEC(4, msg.lights[i].diffuse, light->getDiffuse());
         COPY_VEC(4, msg.lights[i].specular, light->getSpecular());
         COPY_VEC(3, msg.lights[i].spot_direction, light->getDirection());
         msg.lights[i].spot_exponent = light->getSpotExponent();
         msg.lights[i].spot_cutoff = light->getSpotCutoff();
         msg.lights[i].attenuation[0] = light->getConstantAttenuation();
         msg.lights[i].attenuation[1] = light->getLinearAttenuation();
         msg.lights[i].attenuation[2] = light->getQuadraticAttenuation();
      }
      else
      {
          msg.lights[i].enabled = false;
      }

#undef COPY_VEC
   }

   RemoteRenderMessage rrm(msg);
   return remote->send(rrm);
}

//! Task structure for submitting to Intel Threading Building Blocks work //queue
struct DecodeTask: public tbb::task {

   DecodeTask(tbb::concurrent_queue<std::shared_ptr<tileMsg> > &resultQueue, std::shared_ptr<tileMsg> msg, std::shared_ptr<std::vector<char>> payload)
   : resultQueue(resultQueue)
   , msg(msg)
   , payload(payload)
   , rgba(NULL)
   , depth(NULL)
   {}

   tbb::concurrent_queue<std::shared_ptr<tileMsg> > &resultQueue;
   std::shared_ptr<tileMsg> msg;
   std::shared_ptr<std::vector<char>> payload;
   char *rgba, *depth;

   tbb::task* execute() {

      if (!depth && !rgba) {
         // dummy task for other node
         resultQueue.push(msg);
         return NULL;
      }

      size_t sz = 0;
      int bpp = 0;
      if (msg->format == rfbColorRGBA) {
         assert(rgba);
         bpp = 4;
         sz = msg->width * msg->height * bpp;
      } else {
         assert(depth);
         switch (msg->format) {
         case rfbDepthFloat: bpp=4; break;
         case rfbDepth8Bit: bpp=1; break;
         case rfbDepth16Bit: bpp=2; break;
         case rfbDepth24Bit: bpp=4; break;
         case rfbDepth32Bit: bpp=4; break;
         }

         if (msg->compression & rfbTileDepthQuantize) {
            assert(msg->format != rfbDepthFloat);
            sz = depthquant_size(bpp==4 ? DepthFloat : DepthInteger, bpp, msg->width, msg->height);
         } else {
            sz = msg->width * msg->height * bpp;
         }
      }

      if (!payload || sz==0) {
         resultQueue.push(msg);
         return NULL;
      }

      std::shared_ptr<std::vector<char>> decompbuf = payload;
      if (msg->compression & rfbTileSnappy) {
#ifdef HAVE_SNAPPY
         decompbuf.reset(new std::vector<char>(sz));
         snappy::RawUncompress(payload->data(), msg->size, decompbuf->data());
#else
         resultQueue.push(msg);
         return NULL;
#endif
      }

      if (msg->format!=rfbColorRGBA && (msg->compression & rfbTileDepthQuantize)) {

         depthdequant(depth, decompbuf->data(), bpp==4 ? DepthFloat : DepthInteger, bpp, msg->x, msg->y, msg->width, msg->height, msg->totalwidth);
      } else if (msg->compression == rfbTileJpeg) {

#ifdef HAVE_TURBOJPEG
         TjContext::reference tj = tjContexts.local();
         char *dest = msg->format==rfbColorRGBA ? rgba : depth;
         int w=-1, h=-1;
         tjDecompressHeader(tj.handle, reinterpret_cast<unsigned char *>(payload->data()), msg->size, &w, &h);
         dest += (msg->y*msg->totalwidth+msg->x)*bpp;
         int ret = tjDecompress(tj.handle, reinterpret_cast<unsigned char *>(payload->data()), msg->size, reinterpret_cast<unsigned char *>(dest), msg->width, msg->totalwidth*bpp, msg->height, bpp, TJPF_BGR);
         if (ret == -1)
            std::cerr << "RhrClient: JPEG error: " << tjGetErrorStr() << std::endl;
#else
         std::cerr << "RhrClient: no JPEG support" << std::endl;
#endif
      } else {

         char *dest = msg->format==rfbColorRGBA ? rgba : depth;
         for (int yy=0; yy<msg->height; ++yy) {
            memcpy(dest+((msg->y+yy)*msg->totalwidth+msg->x)*bpp, decompbuf->data()+msg->width*yy*bpp, msg->width*bpp);
         }
      }

      resultQueue.push(msg);
      return NULL;
   }
};

// returns true if a frame has been discarded - should be called again
bool RhrClient::updateTileQueue() {

   //std::cerr << "tiles: " << m_queued << " queued, " << m_deferred.size() << " deferred" << std::endl;
   std::shared_ptr<tileMsg> msg;
   while (m_resultQueue.try_pop(msg)) {

      --m_queued;
      assert(m_queued >= 0);

      if (m_queued == 0) {
         if (m_waitForDecode) {
            finishFrame(*msg);
         }
         return false;
      }
   }

   if (m_queued > 0) {
      return false;
   }

   assert(m_waitForDecode == false);

   if (!m_deferred.empty() && !(m_deferred.front()->msg->flags&rfbTileFirst)) {
      std::cerr << "first deferred tile should have rfbTileFirst set" << std::endl;
   }

   assert(m_deferredFrames == 0 || !m_deferred.empty());
   assert(m_deferred.empty() || m_deferredFrames>0);

   while(!m_deferred.empty()) {
      //std::cerr << "emptying deferred queue: tiles=" << m_deferred.size() << ", frames=" << m_deferredFrames << std::endl;
      DecodeTask *dt = m_deferred.front();
      m_deferred.pop_front();

      handleTileMeta(*dt->msg);
      if (dt->msg->flags & rfbTileFirst) {
         --m_deferredFrames;
      }
      bool last = dt->msg->flags & rfbTileLast;
      if (m_deferredFrames > 0) {
         //if (dt->msg->flags & rfbTileFirst) std::cerr << "skipping remote frame" << std::endl;
         tbb::task::destroy(*dt);
         if (last) {
            return true;
         }
      } else {
         //std::cerr << "quueing from updateTaskQueue" << std::endl;
         enqueueTask(dt);
      }
      if (last) {
         break;
      }
   }

   assert(m_deferredFrames == 0 || !m_deferred.empty());
   assert(m_deferred.empty() || m_deferredFrames>0);

   return false;
}

void RhrClient::finishFrame(const tileMsg &msg) {

   m_waitForDecode = false;

   ++m_remoteFrames;
   double delay = cover->frameTime() - msg.requestTime;
   m_accumDelay += delay;
   if (m_minDelay > delay)
      m_minDelay = delay;
   if (m_maxDelay < delay)
      m_maxDelay = delay;
   m_avgDelay = 0.7*m_avgDelay + 0.3*delay;

   m_depthBytesS += m_depthBytes;
   m_rgbBytesS += m_rgbBytes;
   m_depthBppS = m_depthBpp;
   m_numPixelsS = m_numPixels;

   //std::cerr << "finishFrame: t=" << msg.timestep << ", req=" << m_requestedTimestep << std::endl;
   m_remoteTimestep = msg.timestep;
   if (m_requestedTimestep>=0) {
       if (msg.timestep == m_requestedTimestep) {
           m_timestepToCommit = msg.timestep;
       } else if (msg.timestep == m_visibleTimestep) {
           //std::cerr << "finishFrame: t=" << msg.timestep << ", but req=" << m_requestedTimestep  << " - still showing" << std::endl;
       } else {
           std::cerr << "finishFrame: t=" << msg.timestep << ", but req=" << m_requestedTimestep << std::endl;
       }
   }
   m_frameReady = true;
}

bool RhrClient::checkSwapFrame() {

#if 0
   static int count = 0;
   std::cerr << "RhrClient::checkSwapFrame(): " << count << ", frame ready=" << m_frameReady << std::endl;
   ++count;
#endif

   bool doSwap = m_frameReady;
   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sd(sizeof(bool));
      coVRMSController::instance()->readSlaves(&sd);
      for (int s=0; s<coVRMSController::instance()->getNumSlaves(); ++s) {
         bool ready = *static_cast<bool *>(sd.data[s]);
         if (!ready) {
            doSwap = false;
            break;
         }
      }
   } else {
      coVRMSController::instance()->sendMaster(&m_frameReady, sizeof(m_frameReady));
   }
   return coVRMSController::instance()->syncBool(doSwap);
}

void RhrClient::swapFrame() {

   assert(m_frameReady = true);
   m_frameReady = false;

   //assert(m_visibleTimestep == m_remoteTimestep);
   m_remoteTimestep = -1;

   //std::cerr << "frame: timestep=" << m_visibleTimestep << std::endl;

   m_drawer->swapFrame();
}

void RhrClient::handleTileMeta(const tileMsg &msg) {

#ifdef CONNDEBUG
   bool first = msg.flags&rfbTileFirst;
   bool last = msg.flags&rfbTileLast;
   if (first || last) {
      std::cout << "TILE: " << (first?"F":" ") << "." << (last?"L":" ") << "t=" << msg.timestep << "req: " << msg.requestNumber << ", view: " << msg.viewNum << ", frame: " << msg.frameNumber << ", dt: " << cover->frameTime() - msg.requestTime  << std::endl;
   }
#endif
  if (msg.flags & rfbTileFirst) {
      m_numPixels = 0;
      m_depthBytes = 0;
      m_rgbBytes = 0;
   }
   if (msg.format != rfbColorRGBA) {
      m_depthBytes += msg.size;
   } else {
      m_rgbBytes += msg.size;
      m_numPixels += msg.width*msg.height;
   }

   osg::Matrix model, view, proj;
   for (int i=0; i<16; ++i) {
      view.ptr()[i] = msg.view[i];
      proj.ptr()[i] = msg.proj[i];
      model.ptr()[i] = msg.model[i];
   }
   int viewIdx = msg.viewNum - m_channelBase;
   if (viewIdx < 0 || viewIdx >= m_numViews)
      return;
   m_drawer->updateMatrices(viewIdx, model, view, proj);
   int w = msg.totalwidth, h = msg.totalheight;
   m_drawer->resizeView(viewIdx, w, h);

   if (msg.format != rfbColorRGBA) {
      GLenum format = GL_FLOAT;
      switch (msg.format) {
         case rfbDepthFloat: format = GL_FLOAT; m_depthBpp=4; break;
         case rfbDepth8Bit: format = GL_UNSIGNED_BYTE; m_depthBpp=1; break;
         case rfbDepth16Bit: format = GL_UNSIGNED_SHORT; m_depthBpp=2; break;
         case rfbDepth24Bit: format = GL_FLOAT; m_depthBpp=4; break;
         case rfbDepth32Bit: format = GL_FLOAT; m_depthBpp=4; break;
      }
      m_drawer->resizeView(viewIdx, w, h, format);
   }
}

bool RhrClient::canEnqueue() const {

   return !plugin->m_waitForDecode && plugin->m_deferredFrames==0;
}

void RhrClient::enqueueTask(DecodeTask *task) {

   assert(canEnqueue());

   const int view = task->msg->viewNum - m_channelBase;
   const bool last = task->msg->flags & rfbTileLast;

   if (view < 0 || view >= m_numViews) {
      task->rgba = NULL;
      task->depth = NULL;
   } else {
      task->rgba = reinterpret_cast<char *>(m_drawer->rgba(view));
      task->depth = reinterpret_cast<char *>(m_drawer->depth(view));
   }
   if (last) {
      //std::cerr << "waiting for frame finish: x=" << task->msg->x << ", y=" << task->msg->y << std::endl;

      m_waitForDecode = true;
   }
   ++m_queued;
   tbb::task::enqueue(*task);
}

bool RhrClient::handleTileMessage(std::shared_ptr<tileMsg> msg, std::shared_ptr<std::vector<char>> payload) {

#ifdef CONNDEBUG
   bool first = msg->flags & rfbTileFirst;
   bool last = msg->flags & rfbTileLast;

#if 0
   std::cerr << "q=" << m_queued << ", wait=" << m_waitForDecode << ", tile: x="<<msg->x << ", y="<<msg->y << ", w="<<msg->width<<", h="<<msg->height << ", total w=" << msg->totalwidth
      << ", first: " << bool(msg->flags&rfbTileFirst) << ", last: " << last
      << std::endl;
#endif
   std::cerr << "q=" << m_queued << ", wait=" << m_waitForDecode << ", tile: first: " << first << ", last: " << last
      << ", req: " << msg->requestNumber
      << std::endl;
#endif

   if (msg->flags) {
   } else if (msg->viewNum < m_channelBase || msg->viewNum >= m_channelBase+m_numViews) {
       return true;
   }

   DecodeTask *dt = new(tbb::task::allocate_root()) DecodeTask(m_resultQueue, msg, payload);
   if (canEnqueue()) {
      handleTileMeta(*msg);
      enqueueTask(dt);
   } else {
      if (m_deferredFrames == 0) {

         assert(m_deferred.empty());
         assert(msg->flags & rfbTileFirst);
      }
      if (msg->flags & rfbTileFirst)
         ++m_deferredFrames;
      m_deferred.push_back(dt);
   }

   assert(m_deferredFrames == 0 || !m_deferred.empty());
   assert(m_deferred.empty() || m_deferredFrames>0);

   return true;
}


//! called when plugin is loaded
RhrClient::RhrClient()
: m_requestedTimestep(-1)
, m_remoteTimestep(-1)
, m_visibleTimestep(-1)
, m_numRemoteTimesteps(-1)
, m_timestepToCommit(-1)
{
   assert(plugin == NULL);
   plugin = this;
   //fprintf(stderr, "new RhrClient plugin\n");
}
//! called after plug-in is loaded and scenegraph is initialized
bool RhrClient::init()
{
   m_serverHost = covise::coCoviseConfig::getEntry("rfbHost", config, "localhost");
   m_port = covise::coCoviseConfig::getInt("rfbPort", config, 31590);

   m_haveConnection = false;

   m_mode = MultiChannelDrawer::ReprojectMesh;
   m_frameReady = false;
   m_noModelUpdate = false;
   m_oldModelMatrix = osg::Matrix::identity();

#if 0
   std::cout << "COVER waiting for debugger: pid=" << getpid() << std::endl;
   sleep(10);
   std::cout << "COVER continuing..." << std::endl;
#endif

   m_fbImg = new osg::Image();

   m_benchmark = covise::coCoviseConfig::isOn("benchmark", config, true);
   m_compress = covise::coCoviseConfig::getInt("vncCompress", config, 0);
   if (m_compress < 0) m_compress = 0;
   if (m_compress > 9) m_compress = 9;
   m_quality = covise::coCoviseConfig::getInt("vncQuality", config, 9);
   if (m_quality < 0) m_quality = 0;
   if (m_quality > 9) m_quality = 9;
   m_lastStat = cover->currentTime();
   m_avgDelay = 0.;
   m_minDelay = DBL_MAX;
   m_maxDelay = 0.;
   m_accumDelay = 0.;
   m_remoteFrames = 0;
   m_localFrames = 0;
   m_depthBytes = 0;
   m_rgbBytes = 0;
   m_numPixels = 0;
   m_depthBpp = 0;
   m_depthBytesS = 0;
   m_rgbBytesS = 0;
   m_numPixelsS = 0;
   m_depthBppS = 0;

   m_matrixNum = 0;

   m_lastTileAt = -1;
   m_queued = 0;
   m_deferredFrames = 0;
   m_waitForDecode = false;

   m_channelBase = 0;

   const int numChannels = coVRConfig::instance()->numChannels();
   m_numViews = numChannels;
   for (int i=0; i<numChannels; ++i) {
       if (coVRConfig::instance()->channels[i].stereoMode == osg::DisplaySettings::QUAD_BUFFER) {
          ++m_numViews;
       }
   }
   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sd(sizeof(m_numViews));
      coVRMSController::instance()->readSlaves(&sd);
      int channelBase = m_numViews;
      for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
         int *p = static_cast<int *>(sd.data[i]);
         int n = *p;
         m_numChannels.push_back(n);
         *p = channelBase;
         channelBase += n;
      }
      coVRMSController::instance()->sendSlaves(sd);
   } else {
      coVRMSController::instance()->sendMaster(&m_numViews, sizeof(m_numViews));
      coVRMSController::instance()->readMaster(&m_channelBase, sizeof(m_channelBase));
   }

   std::cerr << "numChannels: " << numChannels << ", m_channelBase: " << m_channelBase << std::endl;
   std::cerr << "numViews: " << m_numViews << ", m_channelBase: " << m_channelBase << std::endl;

   m_drawer = new MultiChannelDrawer(numChannels, true /* flip top/bottom */);

#ifdef VRUI
   m_menuItem = new coSubMenuItem("Hybrid Rendering...");
   m_menu = new coRowMenu("Hybrid Rendering");
   m_menuItem->setMenu(m_menu);
   m_menuItem->setMenuListener(this);
   cover->getMenu()->add(m_menuItem);

   m_reprojCheck = new coCheckboxMenuItem("Reproject", m_reproject);
   m_adaptCheck = new coCheckboxMenuItem("Adapt Point Size", m_adapt);
   m_allTilesCheck = new coCheckboxMenuItem("Show all tiles", false);
   m_reprojCheck->setMenuListener(this);
   m_adaptCheck->setMenuListener(this);
   m_allTilesCheck->setMenuListener(this);
   m_menu->add(m_reprojCheck);
   m_menu->add(m_adaptCheck);
   //m_menu->add(m_allTilesCheck);
#else
   m_tab = mui::Tab::create(muiId());
   m_tab->setLabel("Hybrid Rendering");
   m_tab->setEventListener(this);
   m_tab->setPos(1, 0);

   m_reprojCheck = mui::ToggleButton::create(muiId("reproj"), m_tab);
   m_reprojCheck->setLabel("Reproject");
   m_reprojCheck->setEventListener(this);
   m_reprojCheck->setPos(0,0);

   m_adaptCheck = mui::ToggleButton::create(muiId("adapt"), m_tab);
   m_adaptCheck->setLabel("Adapt Point Size");
   m_adaptCheck->setEventListener(this);
   m_adaptCheck->setPos(1,0);

   m_adaptWithNeighborsCheck = mui::ToggleButton::create(muiId("adapt_with_neighbors"), m_tab);
   m_adaptWithNeighborsCheck->setLabel("Adapt With Neighbors");
   m_adaptWithNeighborsCheck->setEventListener(this);
   m_adaptWithNeighborsCheck->setPos(2,0);

   m_asMeshCheck = mui::ToggleButton::create(muiId("as_mesh"), m_tab);
   m_asMeshCheck->setLabel("As Mesh");
   m_asMeshCheck->setEventListener(this);
   m_asMeshCheck->setPos(3,0);

   m_withHolesCheck = mui::ToggleButton::create(muiId("with_holes"), m_tab);
   m_withHolesCheck->setLabel("With Holes");
   m_withHolesCheck->setEventListener(this);
   m_withHolesCheck->setPos(4,0);

   coTUITab *tab = dynamic_cast<coTUITab *>(m_tab->getTUI());

   m_hostLabel = new coTUILabel("Host:", tab->getID());
   m_hostLabel->setEventListener(this);
   m_hostLabel->setPos(0,1);

   m_hostEdit = new coTUIEditTextField("host", tab->getID());
   m_hostEdit->setEventListener(this);
   m_hostEdit->setPos(1,1);
   m_hostEdit->setText(m_serverHost);

   m_portLabel = new coTUILabel("Port:", tab->getID());
   m_portLabel->setEventListener(this);
   m_portLabel->setPos(0,2);

   m_portEdit = new coTUIEditIntField("port", tab->getID());
   m_portEdit->setEventListener(this);
   m_portEdit->setPos(1,2);
   m_portEdit->setValue(m_port);
   m_portEdit->setMin(1);
   m_portEdit->setMax(0xffff);

   m_connectCheck = mui::ToggleButton::create(muiId("connect"), m_tab);
   m_connectCheck->setLabel("Connected");
   m_connectCheck->setEventListener(this);
   m_connectCheck->setPos(0,3);
   m_connectCheck->setState(m_haveConnection);

   m_inhibitModelUpdate = mui::ToggleButton::create(muiId("no_model_update"), m_tab);
   m_inhibitModelUpdate->setLabel("No Model Update");
   m_inhibitModelUpdate->setEventListener(this);
   m_inhibitModelUpdate->setPos(0,4);
   m_inhibitModelUpdate->setState(m_noModelUpdate);
#endif

   m_drawer->setMode(m_mode);
   modeToUi(m_mode);

   return true;
}

//! this is called if the plugin is removed at runtime
RhrClient::~RhrClient()
{
   cover->getScene()->removeChild(m_drawer);

   coVRAnimationManager::instance()->removeTimestepProvider(this);
   if (m_requestedTimestep != -1)
       commitTimestep(m_requestedTimestep);

   clientCleanup(m_remote);

#ifdef VRUI
   delete m_menu;
   delete m_menuItem;
#else
   delete m_tab;
#endif

   plugin = NULL;
}

void RhrClient::modeToUi(MultiChannelDrawer::Mode mode) {
   switch(mode) {
      case MultiChannelDrawer::AsIs:
         m_reproject = false;
         break;
      case MultiChannelDrawer::Reproject:
         m_reproject = true;
         m_adapt = false;
         m_asMesh = false;
         break;
      case MultiChannelDrawer::ReprojectAdaptive:
         m_reproject = true;
         m_adapt = true;
         m_adaptWithNeighbors = false;
         m_asMesh = false;
         break;
      case MultiChannelDrawer::ReprojectAdaptiveWithNeighbors:
         m_reproject = true;
         m_adapt = true;
         m_adaptWithNeighbors = true;
         m_asMesh = false;
         break;
      case MultiChannelDrawer::ReprojectMesh:
         m_reproject = true;
         m_adapt = false;
         m_asMesh = true;
         m_withHoles = false;
         break;
      case MultiChannelDrawer::ReprojectMeshWithHoles:
         m_reproject = true;
         m_adapt = false;
         m_asMesh = true;
         m_withHoles = true;
         break;
   }

   m_reprojCheck->setState(m_reproject);
   m_adaptCheck->setState(m_adapt);
   m_adaptWithNeighborsCheck->setState(m_adaptWithNeighbors);
   m_asMeshCheck->setState(m_asMesh);
   m_withHolesCheck->setState(m_withHoles);
}

void RhrClient::applyMode() {
   MultiChannelDrawer::Mode mode = MultiChannelDrawer::AsIs;
   if (m_reproject) {
      if (m_asMesh) {
         mode = MultiChannelDrawer::ReprojectMesh;
         if (m_withHoles)
            mode = MultiChannelDrawer::ReprojectMeshWithHoles;
      } else {
         if (m_adapt) {
            if (m_adaptWithNeighbors)
               mode = MultiChannelDrawer::ReprojectAdaptiveWithNeighbors;
            else
               mode = MultiChannelDrawer::ReprojectAdaptive;
         } else {
            mode = MultiChannelDrawer::Reproject;
         }
      }
   }
   modeToUi(mode);
   m_mode = mode;
   m_drawer->setMode(m_mode);
}

#ifdef VRUI
void RhrClient::menuEvent(coMenuItem *item) {

   if (item == m_reprojCheck) {
      m_reproject = m_reprojCheck->getState();
      applyMode();
   }
   if (item == m_adaptCheck) {
       m_adapt = m_adaptCheck->getState();
       applyMode();
   }
}
#else
void RhrClient::muiEvent(mui::Element *item) {
   if (item == m_reprojCheck) {
      m_reproject = m_reprojCheck->getState();
      applyMode();
   }
   if (item == m_adaptCheck) {
       m_adapt = m_adaptCheck->getState();
       if (m_adapt)
          m_asMesh = false;
      applyMode();
   }
   if (item == m_adaptWithNeighborsCheck) {
       m_adaptWithNeighbors = m_adaptWithNeighborsCheck->getState();
       if (m_adaptWithNeighbors) {
          m_adapt = true;
          m_asMesh = false;
       }
      applyMode();
   }
   if (item == m_asMeshCheck) {
      m_asMesh = m_asMeshCheck->getState();
      if (m_asMesh)
         m_adapt = false;
      applyMode();
   }
   if (item == m_withHolesCheck) {
      m_withHoles = m_withHolesCheck->getState();
      if (m_withHoles) {
         m_asMesh = true;
         m_adapt = false;
      }
      applyMode();
   }
   if (item == m_connectCheck) {
       if (m_remote && m_remote->isConnected()) {
           clientCleanup(m_remote);
       } else {
           connectClient();
       }
       m_connectCheck->setState(m_haveConnection);
   }
   if (item == m_inhibitModelUpdate) {
       m_noModelUpdate = m_inhibitModelUpdate->getState();
   }
}
void RhrClient::tabletEvent(coTUIElement *item) {
   if (item == m_hostEdit) {
      m_serverHost = m_hostEdit->getText();
   }
   if (item == m_portEdit) {
      m_port = m_portEdit->getValue();
   }
}
#endif

//! this is called before every frame, used for polling for RFB messages
void
RhrClient::preFrame()
{
   static double lastUpdate = -DBL_MAX;
   static double lastMatrices = -DBL_MAX;
   static double lastConnectionTry = -DBL_MAX;
   static int remoteSkipped = 0;

   if (coVRMSController::instance()->isMaster()) {
      m_io.poll();

      if (cover->frameTime() - lastConnectionTry > 3.) {
         lastConnectionTry = cover->frameTime();
         connectClient();
      }
   }

   if (m_remote && !m_remote->isRunning()) {
       clientCleanup(m_remote);
   }

   bool connected = coVRMSController::instance()->syncBool(m_remote && m_remote->isConnected());
   if (m_haveConnection != connected) {
       if (connected)
           cover->getScene()->addChild(m_drawer);
       else
           cover->getScene()->removeChild(m_drawer);
       m_haveConnection = connected;
       m_connectCheck->setState(m_haveConnection);
       if (!connected)
       {
          m_numRemoteTimesteps = -1;
          coVRAnimationManager::instance()->removeTimestepProvider(this);
          if (m_requestedTimestep != -1) {
              commitTimestep(m_requestedTimestep);
              m_requestedTimestep = -1;
          }
       }
   }

   if (!connected)
      return;

   const int numChannels = coVRConfig::instance()->numChannels();
   std::vector<matricesMsg> messages(m_numViews);
   int view=0;
   for (int i=0; i<numChannels; ++i) {

      fillMatricesMessage(messages[view], i, view, false);
      ++view;
      if (coVRConfig::instance()->channels[i].stereoMode==osg::DisplaySettings::QUAD_BUFFER) {
         fillMatricesMessage(messages[view], i, view, true);
         ++view;
      }
   }

   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sNumScreens(sizeof(m_numViews));
      coVRMSController::instance()->readSlaves(&sNumScreens);
      for (int i=0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
         int n = *static_cast<int *>(sNumScreens.data[i]);
         for (int s=0; s<n; ++s) {
            matricesMsg msg;
            coVRMSController::instance()->readSlave(i, &msg, sizeof(msg));
            messages.push_back(msg);
         }
      }
   } else {
      coVRMSController::instance()->sendMaster(&m_numViews, sizeof(m_numViews));
      for (int s=0; s<m_numViews; ++s) {
         coVRMSController::instance()->sendMaster(&messages[s], sizeof(messages[s]));
      }
   }

   int ntiles = 0;
   {
       if (coVRMSController::instance()->isMaster()) {
           if (m_remote->messageReceived()) {
               lastUpdate = cover->frameTime();
           }

           bool sendUpdate = false;
           {
               std::lock_guard<std::mutex> locker(m_pluginMutex);
               sendUpdate = (m_lastTileAt >= 0 || cover->frameTime()-lastMatrices>m_avgDelay*0.7) && m_remote && m_remote->isConnected();
           }
           if (sendUpdate) {
               sendLightsMessage(m_remote);
               sendMatricesMessage(m_remote, messages, m_matrixNum++);
               lastMatrices = cover->frameTime();
           }
       }

       coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
       if (m_numRemoteTimesteps > 0)
           coVRAnimationManager::instance()->setNumTimesteps(m_numRemoteTimesteps, this);
       else
           coVRAnimationManager::instance()->removeTimestepProvider(this);

       {
           std::lock_guard<RemoteConnection> remote_locker(*m_remote);
           std::lock_guard<std::mutex> locker(m_pluginMutex);
           for (auto tile: m_remote->m_receivedTiles) {
               if (tile.msg->flags & rfbTileLast)
                   m_lastTileAt = m_receivedTiles.size();
               m_receivedTiles.push_back(tile);
           }
           m_remote->m_receivedTiles.clear();
       }
       std::lock_guard<std::mutex> locker(m_pluginMutex);
       ntiles = m_receivedTiles.size();
       if (m_lastTileAt >= 0) {
           ntiles = m_lastTileAt;
           m_lastTileAt = -1;
       } else if (ntiles > 100) {
           ntiles = 100;
       }
   }
#ifdef CONNDEBUG
       if (ntiles > 0) {
           std::cerr << "have " << ntiles << " image tiles"
                     << ", last tile for req: " << m_receivedTiles.back().msg->requestNumber
                     << ", last=" << (m_receivedTiles.back().msg->flags & rfbTileLast)
                     << std::endl;
       }
#endif

   const bool broadcastTiles = true;
   const int numSlaves = coVRMSController::instance()->getNumSlaves();
   if (coVRMSController::instance()->isMaster()) {
       std::vector<int> stiles(numSlaves);
       std::vector<std::vector<bool>> forSlave(numSlaves);
       std::vector<void *> md(ntiles), pd(ntiles);
       std::vector<size_t> ms(ntiles), ps(ntiles);
       {
           std::lock_guard<std::mutex> locker(m_pluginMutex);
           for (int i=0; i<ntiles; ++i) {
               TileMessage &tile = m_receivedTiles[i];
               handleTileMessage(tile.msg, tile.payload);
               md[i] = tile.msg.get();
               ms[i] = sizeof(*tile.msg);
               ps[i] = tile.msg->size;
               if (ps[i] > 0)
                   pd[i] = tile.payload->data();
           }

           if (!broadcastTiles) {
               int channelBase = coVRConfig::instance()->numChannels();
               for (int s=0; s<numSlaves; ++s) {
                   const int numChannels = m_numChannels[s];
                   stiles[s] = 0;
                   forSlave[s].resize(ntiles);
                   for (int i=0; i<ntiles; ++i) {
                       TileMessage &tile = m_receivedTiles[i];
                       forSlave[s][i] = false;
                       //std::cerr << "ntiles=" << ntiles << ", have=" << m_receivedTiles.size() << ", i=" << i << std::endl;
                       if (tile.msg->flags & rfbTileFirst || tile.msg->flags & rfbTileLast) {
                       } else if (tile.msg->viewNum < channelBase || tile.msg->viewNum >= channelBase+numChannels) {
                           continue;
                       }
                       forSlave[s][i] = true;
                       ++stiles[s];
                   }
                   channelBase += numChannels;
               }
           }
       }
       if (broadcastTiles) {
           //std::cerr << "broadcasting " << ntiles << " tiles" << std::endl;
           coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
           for (int i=0; i<ntiles; ++i) {
               assert(ms[i] == sizeof(tileMsg));
               coVRMSController::instance()->syncData(md[i], ms[i]);
               if (ps[i] > 0) {
                   coVRMSController::instance()->syncData(pd[i], ps[i]);
               }
           }
       } else {
           for (int s=0; s<numSlaves; ++s) {
               //std::cerr << "unicasting " << stiles[s] << " tiles to slave " << s << " << std::endl;
               coVRMSController::instance()->sendSlave(s, &stiles[s], sizeof(stiles[s]));
               for (int i=0; i<ntiles; ++i) {
                   if (forSlave[s][i]) {
                       coVRMSController::instance()->sendSlave(s, md[i], ms[i]);
                       if (ps[i] > 0) {
                           coVRMSController::instance()->sendSlave(s, pd[i], ps[i]);
                       }
                   }
               }
           }
       }

       std::lock_guard<std::mutex> locker(m_pluginMutex);
       for (int i=0;i<ntiles;++i) {
           m_receivedTiles.pop_front();
       }
       if (m_lastTileAt >= 0)
           m_lastTileAt -= ntiles;
       //assert(m_receivedTiles.empty());
   } else {
       if (broadcastTiles) {
          coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
          for (int i=0; i<ntiles; ++i) {
             std::shared_ptr<tileMsg> msg(new tileMsg);
             coVRMSController::instance()->syncData(msg.get(), sizeof(*msg));
             std::shared_ptr<std::vector<char>> payload;
             if (msg->size > 0) {
                payload.reset(new std::vector<char>(msg->size));
                coVRMSController::instance()->syncData(payload->data(), msg->size);
             }
             handleTileMessage(msg, payload);
          }
       } else {
          coVRMSController::instance()->readMaster(&ntiles, sizeof(ntiles));
          //std::cerr << "receiving " << ntiles << " tiles" << std::endl;
          for (int i=0; i<ntiles; ++i) {
             std::shared_ptr<tileMsg> msg(new tileMsg);
             coVRMSController::instance()->readMaster(msg.get(), sizeof(*msg));
             std::shared_ptr<std::vector<char>> payload;
             if (msg->size > 0) {
                payload.reset(new std::vector<char>(msg->size));
                coVRMSController::instance()->readMaster(payload->data(), msg->size);
             }
             handleTileMessage(msg, payload);
          }
       }
   }

#if 0
   if (m_client && cover->frameTime() - lastUpdate > 5.) {
      //SendFramebufferUpdateRequest(m_client, 0, 0, m_client->width, m_client->height, FALSE);
      SendFramebufferUpdateRequest(m_client, 0, 0, 1, 1, FALSE);
      lastUpdate = cover->frameTime();
   }
#endif
   while (updateTileQueue())
      ++remoteSkipped;

   if (checkSwapFrame()) {
       coVRMSController::instance()->syncData(&m_timestepToCommit, sizeof(m_timestepToCommit));
       if (m_timestepToCommit >= 0) {
           //std::cerr << "RhrClient::commitTimestep(" << m_remoteTimestep << ") B" << std::endl;
           commitTimestep(m_timestepToCommit);
           m_timestepToCommit = -1;
       } else {
           swapFrame();
       }
   }

   ++m_localFrames;
   double diff = cover->frameTime() - m_lastStat;
   if (plugin->m_benchmark && diff > 3.) {

      if (coVRMSController::instance()->isMaster()) {
         fprintf(stderr, "depth bpp: %ld, depth bytes: %ld, rgb bytes: %ld\n", 
               (long)m_depthBppS, (long)m_depthBytesS, (long)m_rgbBytesS);
         double depthRate = (double)m_depthBytesS/((m_remoteFrames)*m_depthBppS*m_numPixelsS);
         double rgbRate = (double)m_rgbBytesS/((m_remoteFrames)*3*m_numPixelsS);
         double bandwidthMB = (m_depthBytesS+m_rgbBytesS)/diff/1024/1024;
         if (m_remoteFrames + remoteSkipped > 0) {
            fprintf(stderr, "VNC RHR: FPS: local %f, remote %f, SKIPPED: %d, DELAY: min %f, max %f, avg %f\n",
                  m_localFrames/diff, (m_remoteFrames+remoteSkipped)/diff,
                  remoteSkipped,
                  m_minDelay, m_maxDelay, m_accumDelay/m_remoteFrames);
         } else {
            fprintf(stderr, "VNC RHR: FPS: local %f, remote %f\n",
                  m_localFrames/diff, (m_remoteFrames+remoteSkipped)/diff);
         }
         fprintf(stderr, "    pixels: %ld, bandwidth: %.2f MB/s",
               (long)m_numPixelsS, bandwidthMB);
         if (m_remoteFrames > 0)
            fprintf(stderr, ", depth comp: %.1f%%, rgb comp: %.1f%%",
                  depthRate*100, rgbRate*100);
         fprintf(stderr, "\n");
      }

      remoteSkipped = 0;
      m_lastStat = cover->frameTime();
      m_minDelay = DBL_MAX;
      m_maxDelay = 0.;
      m_accumDelay = 0.;
      m_localFrames = 0;
      m_remoteFrames = 0;
      m_depthBytesS = 0;
      m_rgbBytesS = 0;
   }

   if (cover->isHighQuality()) {
       m_drawer->setMode(MultiChannelDrawer::AsIs);
   } else {
       if (m_drawer->mode() != m_mode)
           m_drawer->setMode(m_mode);
       const osg::Matrix &transform = cover->getXformMat();
       const osg::Matrix &scale = cover->getObjectsScale()->getMatrix();
       int viewIdx = 0;
       for (int i=0; i<coVRConfig::instance()->numChannels(); ++i) {
           const channelStruct &chan = coVRConfig::instance()->channels[i];
           const bool left = chan.stereoMode == osg::DisplaySettings::LEFT_EYE;
           const osg::Matrix &view = left ? chan.leftView : chan.rightView;
           const osg::Matrix &proj = left ? chan.leftProj : chan.rightProj;
           const osg::Matrix model = scale * transform;
           m_drawer->reproject(viewIdx, model, view, proj);
           ++viewIdx;

           if (chan.stereoMode == osg::DisplaySettings::QUAD_BUFFER) {
               const osg::Matrix &view = chan.leftView;
               const osg::Matrix &proj = chan.leftProj;
               m_drawer->reproject(viewIdx, model, view, proj);
               ++viewIdx;
           }
       }
   }
}

//! called when scene bounding sphere is required
void RhrClient::expandBoundingSphere(osg::BoundingSphere &bs) {

    if (coVRMSController::instance()->isMaster()) {

        if (m_remote && m_remote->requestBounds()) {

            double start = cover->currentTime();
            while(m_remote->isRunning()) {
                double elapsed = cover->currentTime() - start;
                if (elapsed > 2.) {
                    start = cover->currentTime();
#if 1
                    std::cerr << "RhrClient: still waiting for bounding sphere from server" << std::endl;
#else
                    std::cerr << "RhrClient: re-requesting bounding sphere from server" << std::endl;

                    m_remote->requestBounds();
#endif
                }
                usleep(1000);
                if (m_remote->boundsUpdated()) {
                    break;
                }
            }
            if (m_remote->boundsUpdated()) {
                bs.expandBy(m_remote->boundsNode->getBound());
            }
        }
    }
    coVRMSController::instance()->syncData(&bs, sizeof(bs));
}

void RhrClient::setTimestep(int t) {

   //std::cerr << "setTimestep(" << t << ")" << std::endl;
    if (m_requestedTimestep == t) {
        m_requestedTimestep = -1;
    }
    if (m_requestedTimestep >= 0) {
        std::cerr << "setTimestep(" << t << "), but requested was " << m_requestedTimestep << std::endl;
    }
   m_visibleTimestep = t;
   if (checkSwapFrame())
       swapFrame();
}

void RhrClient::requestTimestep(int t) {

    //std::cerr << "m_requestedTimestep: " << m_requestedTimestep << " -> " << t << std::endl;
    if (t < 0)
        return;

    if (m_remoteTimestep == t) {
        //std::cerr << "RhrClient::commitTimestep(" << t << ") immed" << std::endl;
        m_requestedTimestep = -1;
        commitTimestep(t);
#if 0
    } else if (m_visibleTimestep==t && m_remoteTimestep==-1) {
        m_requestedTimestep = -1;
        commitTimestep(t);
#endif
    } else if (t != m_requestedTimestep) {
        m_requestedTimestep = t;
        bool requested = false;
        if (m_remote) {
            if (coVRMSController::instance()->isMaster()) {
                animationMsg msg;
                msg.current = t;
                msg.total = coVRAnimationManager::instance()->getNumTimesteps();
                RemoteRenderMessage rrm(msg);
                requested = m_remote->send(rrm);
            }
            requested = coVRMSController::instance()->syncBool(requested);
        }
        if (!requested) {
            commitTimestep(t);
        }
    }
}

bool RhrClient::connectClient() {

   if (m_remote)
      return true;

   m_avgDelay = 0.;

   std::cerr << "starting new RemoteConnection" << std::endl;
   m_remote.reset(new RemoteConnection(this, m_serverHost, m_port, coVRMSController::instance()->isMaster()));

   return true;
}

//! clean up when connection to server is lost
void RhrClient::clientCleanup(std::shared_ptr<RemoteConnection> &remote) {

   if (remote) {
       remote->stopThread();
   }
   remote.reset();
}

COVERPLUGIN(RhrClient)
