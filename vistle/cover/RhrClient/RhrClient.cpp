/**\file
 * \brief RhrClient plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 *
 * \copyright GPL2+
 */

const bool c_AllowModelTransformationSetting = true;
const bool c_AllowOwnInput = false;

#include "RhrClient.h"

#include <config/CoviseConfig.h>

#include <net/tokenbuffer.h>

#include <cover/coVRConfig.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRMSController.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coVRPluginList.h>
#include <cover/VRViewer.h>
#include <cover/VRSceneGraph.h>
#include <cover/coVRLighting.h>
#include <cover/coVRAnimationManager.h>
#include <cover/RenderObject.h>

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

#include <rhr/rfbext.h>


#include <tbb/parallel_for.h>
#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>
#include <memory>

#include <thread>
#include <mutex>
#include <map>

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

#ifdef HAVE_ZFP
#include <zfp.h>
#endif

using message::RemoteRenderMessage;
#include <core/tcpmessage.h>

#include <VistlePluginUtil/VistleRenderObject.h>

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
    std::recursive_mutex *m_mutex=nullptr, *m_sendMutex=nullptr;
    std::thread *m_thread = nullptr;
    bool haveMessage = false;
    bool m_running = false;
    bool m_connected = false;
    bool m_interrupt = false;
    bool m_isMaster = false;
    std::deque<TileMessage> m_receivedTiles;
    std::map<std::string, vistle::RenderObject::InitialVariantVisibility> m_variantsToAdd;
    std::set<std::string> m_variantsToRemove;
    std::map<std::string, std::shared_ptr<VariantRenderObject>> m_variants;
    int m_requestedTimestep=-1, m_remoteTimestep=-1, m_visibleTimestep=-1, m_numRemoteTimesteps=-1, m_timestepToCommit=-1;
    osg::ref_ptr<opencover::MultiChannelDrawer> m_drawer;

    RemoteConnection() = delete;
    RemoteConnection(const RemoteConnection& other) = delete;
    RemoteConnection(RhrClient *plugin, std::string host, unsigned short port, bool isMaster)
        : plugin(plugin)
        , m_host(host)
        , m_port(port)
        , m_sock(plugin->m_io)
        , m_isMaster(isMaster)
    {
        boundsNode = new osg::Node;
        m_mutex = new std::recursive_mutex;
        m_sendMutex = new std::recursive_mutex;
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
        delete m_sendMutex;
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

        {
            lock_guard locker(*m_mutex);
            m_connected = true;
            connectionEstablished();
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
                case rfbAnimation: {
                    handleAnimation(msg, static_cast<const animationMsg &>(rhr));
                    break;
                }
                case rfbVariant: {
                    handleVariant(msg, static_cast<const variantMsg &>(rhr));
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

    void connectionEstablished() {
        std::lock_guard<std::mutex> locker(plugin->m_pluginMutex);
        for (const auto &var: plugin->m_coverVariants) {
            variantMsg msg;
            strncpy(msg.name, var.first.c_str(), sizeof(msg.name));
            msg.visible = var.second;
            send(msg);
        }

        animationMsg msg;
        msg.current = plugin->m_visibleTimestep;
        msg.total = coVRAnimationManager::instance()->getNumTimesteps();
        send(msg);
    }

    bool handleAnimation(const RemoteRenderMessage &msg, const animationMsg &anim) {
        lock_guard locker(*m_mutex);
        m_numRemoteTimesteps = anim.total;
        return true;
    }

    bool handleVariant(const RemoteRenderMessage &msg, const variantMsg &variant) {
        std::string name = variant.name;
        if (variant.remove) {
            m_variantsToRemove.emplace(name);
            std::cerr << "Variant: remove " << name << std::endl;
        } else {
            vistle::RenderObject::InitialVariantVisibility vis = vistle::RenderObject::DontChange;
            if (variant.configureVisibility)
                vis = variant.visible ? vistle::RenderObject::Visible : vistle::RenderObject::Hidden;
            m_variantsToAdd.emplace(name, vis);
            std::cerr << "Variant: add " << name << std::endl;
        }
        return true;
    }
    void update() {
        lock_guard locker(*m_mutex);
        updateVariants();
        coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
        plugin->m_numRemoteTimesteps = m_numRemoteTimesteps;
        coVRMSController::instance()->syncData(&m_remoteTimestep, sizeof(m_remoteTimestep));
        plugin->m_remoteTimestep = m_remoteTimestep;
    }

    void updateVariants() {
        int numAdd = m_variantsToAdd.size(), numRemove = m_variantsToRemove.size();
        coVRMSController::instance()->syncData(&numAdd, sizeof(numAdd));
        coVRMSController::instance()->syncData(&numRemove, sizeof(numRemove));
        auto itAdd = m_variantsToAdd.begin();
        for (int i=0; i<numAdd; ++i) {
            std::string s;
            enum vistle::RenderObject::InitialVariantVisibility vis;
            if (m_isMaster) {
                s = itAdd->first;
                vis = itAdd->second;
            }
            s = coVRMSController::instance()->syncString(s);
            coVRMSController::instance()->syncData(&vis, sizeof(vis));
            if (!m_isMaster) {
                m_variantsToAdd.emplace(s, vis);
            } else {
                ++itAdd;
            }
        }
        assert(m_variantsToAdd.size() == numAdd);
        auto itRemove = m_variantsToRemove.begin();
        for (int i=0; i<numRemove; ++i) {
            std::string s;
            if (m_isMaster)
                s = *itRemove;
            s = coVRMSController::instance()->syncString(s);
            if (!m_isMaster) {
                m_variantsToRemove.insert(s);
            } else {
                ++itRemove;
            }
        }
        assert(m_variantsToRemove.size() == numRemove);

        if (!m_variantsToAdd.empty())
            cover->addPlugin("Variant");
        for (auto &var: m_variantsToAdd) {
            auto it = m_variants.find(var.first);
            if (it == m_variants.end()) {
                it = m_variants.emplace(var.first, std::make_shared<VariantRenderObject>(var.first, var.second)).first;
            }
            coVRPluginList::instance()->addNode(it->second->node(), it->second.get(), plugin);
        }
        m_variantsToAdd.clear();

        for (auto &var: m_variantsToRemove) {
            auto it = m_variants.find(var);
            if (it != m_variants.end()) {
                coVRPluginList::instance()->removeNode(it->second->node(), it->second.get());
                m_variants.erase(it);
            }
        }
        m_variantsToRemove.clear();
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

    bool isRunning() const {
        lock_guard locker(*m_mutex);
        return m_running;
    }

    bool isConnecting() const {
        lock_guard locker(*m_mutex);
        return !m_connected && m_sock.is_open();
    }

    bool isConnected() const {
        lock_guard locker(*m_mutex);
        return m_connected && m_sock.is_open();
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

    bool sendMessage(const message::Message &msg, const std::vector<char> *payload=nullptr) {
        lock_guard locker(*m_sendMutex);
        if (m_sock.is_open()) {
            bool ok = message::send(m_sock, msg, payload);
            if (!ok)
                m_sock.close();
            return ok;
        }
        return false;
    }

    template<class Message>
    bool send(const Message &msg, const std::vector<char> *payload=nullptr) {
        RemoteRenderMessage r(msg, payload ? payload->size() : 0);
        return sendMessage(r, payload);
    }

    bool requestBounds() {
        lock_guard locker(*m_mutex);
        if (!isConnected())
            return false;
        m_boundsUpdated = false;

        boundsMsg msg;
        msg.type = rfbBounds;
        msg.sendreply = 1;
        return send(msg);
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

   if (m_mode == MultiChannelDrawer::ReprojectCubemap)
   {
	   m_drawer->GetCubemapReprojector()->AdjustDimensionsAndMatrices(
		   viewNum, msg.width, msg.height, &msg.width, &msg.height,
		   chan.leftView.ptr(), chan.rightView.ptr(),
		   chan.leftProj.ptr(), chan.rightProj.ptr(), msg.view, msg.proj);
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
      if (!remote->send(msg))
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

   return remote->send(msg);
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

      size_t sz = msg->unzippedsize;
      int bpp = 0;
      if (msg->format == rfbColorRGBA) {
         assert(rgba);
         bpp = 4;
      } else {
         assert(depth);
         switch (msg->format) {
         case rfbDepthFloat: bpp=4; break;
         case rfbDepth8Bit: bpp=1; break;
         case rfbDepth16Bit: bpp=2; break;
         case rfbDepth24Bit: bpp=4; break;
         case rfbDepth32Bit: bpp=4; break;
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

      if (msg->format!=rfbColorRGBA && (msg->compression & rfbTileDepthZfp)) {

#ifndef HAVE_ZFP
          std::cerr << "RhrClient: not compiled with zfp support, cannot decompress" << std::endl;
#else
          if (msg->format==rfbDepthFloat) {
              zfp_type type = zfp_type_float;
              bitstream *stream = stream_open(decompbuf->data(), decompbuf->size());
              zfp_stream *zfp = zfp_stream_open(stream);
              zfp_stream_rewind(zfp);
              zfp_field *field = zfp_field_2d(depth, type, msg->width, msg->height);
              if (!zfp_read_header(zfp, field, ZFP_HEADER_FULL)) {
                  std::cerr << "RhrClient: reading zfp compression parameters failed" << std::endl;
              }
              if (field->type != type) {
                  std::cerr << "RhrClient: zfp type not float" << std::endl;
              }
              if (field->nx != msg->width || field->ny != msg->height) {
                  std::cerr << "RhrClient: zfp size mismatch: " << field->nx << "x" << field->ny << " != " << msg->width << "x" << msg->height << std::endl;
              }
              zfp_field_set_pointer(field, depth+(msg->y*msg->totalwidth+msg->x)*bpp);
              zfp_field_set_stride_2d(field, 1, msg->totalwidth);
              if (!zfp_decompress(zfp, field)) {
                  std::cerr << "RhrClient: zfp decompression failed" << std::endl;
              }
              zfp_stream_close(zfp);
              zfp_field_free(field);
              stream_close(stream);
          } else {
              std::cerr << "RhrClient: zfp not in float format, cannot decompress" << std::endl;
          }
#endif
      } else  if (msg->format!=rfbColorRGBA && (msg->compression & rfbTileDepthQuantize)) {

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
   m_remote->m_remoteTimestep = msg.timestep;
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
         default: std::cerr << "unhandled image format " << msg.format << std::endl;
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

   if (msg->flags || m_mode == MultiChannelDrawer::ReprojectCubemap) {
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

unsigned RhrClient::GetCountChannels() const
{
	return (m_mode == MultiChannelDrawer::ReprojectCubemap
		? m_numViews
		: coVRConfig::instance()->numChannels());
}

void RhrClient::SetNumViewsAndChannelBase()
{
	if (m_mode == MultiChannelDrawer::ReprojectCubemap)
	{
		m_numViews = m_drawer->GetCubemapReprojector()->GetCountSourceViews();
		m_channelBase = 0;
	}
	else
	{
		const int numChannels = coVRConfig::instance()->numChannels();
		m_numViews = numChannels;
		for (int i = 0; i < numChannels; ++i)
			if (coVRConfig::instance()->channels[i].stereoMode == osg::DisplaySettings::QUAD_BUFFER)
				++m_numViews;

		if (coVRMSController::instance()->isMaster())
		{
			coVRMSController::SlaveData sd(sizeof(m_numViews));
			coVRMSController::instance()->readSlaves(&sd);
			int channelBase = m_numViews;
			for (int i = 0; i<coVRMSController::instance()->getNumSlaves(); ++i) {
				int *p = static_cast<int *>(sd.data[i]);
				int n = *p;
				m_numChannels.push_back(n);
				*p = channelBase;
				channelBase += n;
			}
			coVRMSController::instance()->sendSlaves(sd);
			m_channelBase = 0;
		}
		else {
			coVRMSController::instance()->sendMaster(&m_numViews, sizeof(m_numViews));
			coVRMSController::instance()->readMaster(&m_channelBase, sizeof(m_channelBase));
		}
	}
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

   SetNumViewsAndChannelBase();

   std::cerr << "numChannels: " << GetCountChannels() << ", m_channelBase: " << m_channelBase << std::endl;
   std::cerr << "numViews: " << m_numViews << ", m_channelBase: " << m_channelBase << std::endl;

   m_drawer = new MultiChannelDrawer(GetCountChannels(), true /* flip top/bottom */);

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

   m_CubemapReprojectionCheck = mui::ToggleButton::create(muiId("as_a_cubemap"), m_tab);
   m_CubemapReprojectionCheck->setLabel("As a cubemap");
   m_CubemapReprojectionCheck->setEventListener(this);
   m_CubemapReprojectionCheck->setPos(5, 0);

   //coTUITab *tab = dynamic_cast<coTUITab *>(m_tab->getTUI());

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

void RhrClient::addObject(const opencover::RenderObject *baseObj, osg::Group *parent, const opencover::RenderObject *geometry, const opencover::RenderObject *normals, const opencover::RenderObject *colors, const opencover::RenderObject *texture)
{

    if (!baseObj)
        return;
    auto attr = baseObj->getAttribute("_rhr_config");
    if (!attr)
        return;
    std::cerr << "RhrClient: connection config string=" << attr << std::endl;

    std::stringstream config(attr);
    unsigned short port;
    std::string method, address;
    config >> method >> address >> port;
    std::cerr << "RhrClient: connection config: method=" << method << ", address=" << address << ", port=" << port << std::endl;

    if (method == "connect") {
        if (address.empty()) {
            std::cerr << "RhrClient: no connection attempt: invalid dest address: " << address << std::endl;
        } else if (port == 0) {
            std::cerr << "RhrClient: no connection attempt: invalid dest port: " << port << std::endl;
        } else {
            connectClient(baseObj->getName(), address, port);
        }
    }
}

void RhrClient::removeObject(const char *objName, bool replaceFlag) {
    if (!objName)
        return;
    const std::string name(objName);
    auto it = m_remotes.find(name);
    if (it == m_remotes.end())
        return;
    clientCleanup(m_remote);
    m_remotes.erase(it);
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
		 m_ReprojectAsCubemap = false;
         break;
      case MultiChannelDrawer::ReprojectAdaptive:
         m_reproject = true;
         m_adapt = true;
         m_adaptWithNeighbors = false;
         m_asMesh = false;
		 m_ReprojectAsCubemap = false;
         break;
      case MultiChannelDrawer::ReprojectAdaptiveWithNeighbors:
         m_reproject = true;
         m_adapt = true;
         m_adaptWithNeighbors = true;
         m_asMesh = false;
		 m_ReprojectAsCubemap = false;
         break;
      case MultiChannelDrawer::ReprojectMesh:
         m_reproject = true;
         m_adapt = false;
         m_asMesh = true;
         m_withHoles = false;
		 m_ReprojectAsCubemap = false;
         break;
      case MultiChannelDrawer::ReprojectMeshWithHoles:
         m_reproject = true;
         m_adapt = false;
         m_asMesh = true;
         m_withHoles = true;
		 m_ReprojectAsCubemap = false;
         break;
	  case MultiChannelDrawer::ReprojectCubemap:
		  m_reproject = true;
		  m_adapt = false;
		  m_asMesh = true;
		  m_withHoles = false;
		  m_ReprojectAsCubemap = true;
		  break;
   }

   m_reprojCheck->setState(m_reproject);
   m_adaptCheck->setState(m_adapt);
   m_adaptWithNeighborsCheck->setState(m_adaptWithNeighbors);
   m_asMeshCheck->setState(m_asMesh);
   m_withHolesCheck->setState(m_withHoles);
   m_CubemapReprojectionCheck->setState(m_ReprojectAsCubemap);
}

void RhrClient::applyMode() {
   MultiChannelDrawer::Mode mode = MultiChannelDrawer::AsIs;
   if (m_reproject) {
      if (m_asMesh) {
         mode = MultiChannelDrawer::ReprojectMesh;
         if (m_withHoles)
            mode = MultiChannelDrawer::ReprojectMeshWithHoles;
		 if (m_ReprojectAsCubemap)
		 {
			 mode = MultiChannelDrawer::ReprojectCubemap;
		 }
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

   SetNumViewsAndChannelBase();

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
   if (item == m_CubemapReprojectionCheck)
   {
	   m_ReprojectAsCubemap = m_CubemapReprojectionCheck->getState();
	   if (m_ReprojectAsCubemap)
	   {
		   m_reproject = true;
		   m_adapt = false;
		   m_asMesh = true;
		   m_withHoles = false;
	   }
	   applyMode();
   }
   if (item == m_inhibitModelUpdate) {
       m_noModelUpdate = m_inhibitModelUpdate->getState();
   }
}
void RhrClient::tabletEvent(coTUIElement *item) {
}
#endif

//! this is called before every frame, used for polling for RFB messages
void
RhrClient::preFrame()
{
	// DEBUG!!!!
	if (c_AllowModelTransformationSetting)
	{
		double model[16];

		static bool frameSetting_IsFirstFrame = true;
		if (frameSetting_IsFirstFrame)
		{
			frameSetting_IsFirstFrame = false;
			m_drawer->GetCubemapReprojector()->GetFirstModelMatrix(model);
			cover->setXformMat(osg::Matrix(model));
			m_drawer->GetCubemapReprojector()->SetClientCameraMovementAllowed(c_AllowOwnInput);
		}
		else
		{
			if (m_mode == MultiChannelDrawer::Mode::ReprojectCubemap && c_AllowOwnInput)
			{
				bool isValid = false;
				m_drawer->GetCubemapReprojector()->GetModelMatrix(model, &isValid);
				if(isValid)
					cover->setXformMat(osg::Matrix(model));
			}
		}
	}

   static double lastMatrices = -DBL_MAX;
   static int remoteSkipped = 0;

   m_io.poll();

   bool running = coVRMSController::instance()->syncBool(m_remote && m_remote->isRunning());
   if (m_remote && !running) {
       clientCleanup(m_remote);
   }

   bool connected = coVRMSController::instance()->syncBool(m_remote && m_remote->isConnected());
   if (m_haveConnection != connected) {
       if (connected)
           cover->getScene()->addChild(m_drawer);
       else
           cover->getScene()->removeChild(m_drawer);
       m_haveConnection = connected;
       if (!connected)
       {
          std::lock_guard<std::mutex> locker(m_pluginMutex);
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

   m_remote->update();

   std::vector<matricesMsg> messages(m_numViews);
   if (m_mode == MultiChannelDrawer::ReprojectCubemap)
   {
		assert(m_numViews = m_drawer->GetCubemapReprojector()->GetCountSourceViews());
	   for (int i = 0; i < m_numViews; ++i) fillMatricesMessage(messages[i], 0, i, false);
   }
   else
   {
	   const int numChannels = coVRConfig::instance()->numChannels();
	   int view = 0;
	   for (int i = 0; i < numChannels; ++i) {

		   fillMatricesMessage(messages[view], i, view, false);
		   ++view;
		   if (coVRConfig::instance()->channels[i].stereoMode == osg::DisplaySettings::QUAD_BUFFER) {
			   fillMatricesMessage(messages[view], i, view, true);
			   ++view;
		   }
	   }

	   if (coVRMSController::instance()->isMaster()) {
		   coVRMSController::SlaveData sNumScreens(sizeof(m_numViews));
		   coVRMSController::instance()->readSlaves(&sNumScreens);
		   for (int i = 0; i < coVRMSController::instance()->getNumSlaves(); ++i) {
			   int n = *static_cast<int *>(sNumScreens.data[i]);
			   for (int s = 0; s < n; ++s) {
				   matricesMsg msg;
				   coVRMSController::instance()->readSlave(i, &msg, sizeof(msg));
				   messages.push_back(msg);
			   }
		   }
	   }
	   else {
		   coVRMSController::instance()->sendMaster(&m_numViews, sizeof(m_numViews));
		   for (int s = 0; s < m_numViews; ++s) {
			   coVRMSController::instance()->sendMaster(&messages[s], sizeof(messages[s]));
		   }
	   }
   }

   int ntiles = 0;
   {
       if (coVRMSController::instance()->isMaster()) {
           bool sendUpdate = false;
           {
               bool changed = messages.size() != m_oldMatrices.size();
               for (size_t i=0; !changed && i<messages.size(); ++i) {
                   const auto &cur = messages[i], &old = m_oldMatrices[i];
                   for (int j=0; j<16; ++j) {
                       if (cur.view[j] != old.view[j])
                           changed = true;
                       if (cur.model[j] != old.model[j])
                           changed = true;
                       if (cur.proj[j] != old.proj[j])
                           changed = true;
                   }
               }
               if (changed) {
                   std::lock_guard<std::mutex> locker(m_pluginMutex);
                   if (m_remote && m_remote->isConnected())
                       sendUpdate = m_lastTileAt >= 0
                               || cover->frameTime()-lastMatrices>m_avgDelay*0.7
                               || cover->frameTime()-lastMatrices>0.3;
               }
           }
           if (sendUpdate) {
               sendLightsMessage(m_remote);
               sendMatricesMessage(m_remote, messages, m_matrixNum++);
               lastMatrices = cover->frameTime();
               std::swap(m_oldMatrices, messages);
           }
       }

       {
           std::lock_guard<std::mutex> locker(m_pluginMutex);
           coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
           if (m_numRemoteTimesteps > 0)
               coVRAnimationManager::instance()->setNumTimesteps(m_numRemoteTimesteps, this);
           else
               coVRAnimationManager::instance()->removeTimestepProvider(this);
       }

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
            fprintf(stderr, "VNC RHR: #Mat updates: %d, FPS: local %f, remote %f, SKIPPED: %d, DELAY: min %f, max %f, avg %f\n",
                  m_matrixNum,
                  m_localFrames/diff, (m_remoteFrames+remoteSkipped)/diff,
                  remoteSkipped,
                  m_minDelay, m_maxDelay, m_accumDelay/m_remoteFrames);
         } else {
            fprintf(stderr, "VNC RHR: FPS: #Mat updates: %d, local %f, remote %f\n",
                  m_matrixNum,
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
    if (t < 0) {
        commitTimestep(t);
        return;
    }

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
        bool requested = false;
        if (m_remote && m_remote->isConnected()) {
            animationMsg msg;
            msg.current = t;
            msg.total = coVRAnimationManager::instance()->getNumTimesteps();
            requested = m_remote->send(msg);
        }
        requested = coVRMSController::instance()->syncBool(requested);
        if (requested) {
            m_requestedTimestep = t;
        }
        if (!requested || m_numRemoteTimesteps <= t) {
            commitTimestep(t);
        }
    }
}

void RhrClient::message(int type, int len, const void *msg) {

    switch (type) {
    case PluginMessageTypes::VariantHide:
    case PluginMessageTypes::VariantShow: {
        covise::TokenBuffer tb((char *)msg, len);
        std::string VariantName;
        tb >> VariantName;
        bool visible = type==PluginMessageTypes::VariantShow;
        m_coverVariants[VariantName] = visible;
        if (coVRMSController::instance()->isMaster()) {
            variantMsg msg;
            msg.visible = visible;
            strncpy(msg.name, VariantName.c_str(), sizeof(msg.name));
            if (m_remote && m_remote->isConnected())
                m_remote->send(msg);
        }
        break;
    }
    }
}

bool RhrClient::connectClient(const std::string &connectionName, const std::string &address, unsigned short port) {

   m_avgDelay = 0.;

   std::cerr << "starting new RemoteConnection to " << address << ":" << port << std::endl;
   m_remote.reset(new RemoteConnection(this, address, port, coVRMSController::instance()->isMaster()));
   m_remotes[connectionName] = m_remote;

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
