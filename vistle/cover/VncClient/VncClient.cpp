/**\file
 * \brief VncClient plugin class
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

#include <PluginUtil/PluginMessageTypes.h>

#include <OpenVRUI/coSubMenuItem.h>
#include <OpenVRUI/coRowMenu.h>
#include <OpenVRUI/coCheckboxMenuItem.h>
#include <OpenVRUI/coPotiMenuItem.h>

#include <osgGA/GUIEventAdapter>
#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>
#include <OpenThreads/Condition>
#include <osg/Material>
#include <osg/TexEnv>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/TextureRectangle>
#include <osgViewer/Renderer>

#include <osg/io_utils>

#include <rfb/rfb.h>
#include <rfb/rfbclient.h>

#include "VncClient.h"

#include <rhr/rfbext.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "RemoteRenderObject.h"
#include "coRemoteCoviseInteractor.h"

#include <tbb/parallel_for.h>
#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>
#include <boost/shared_ptr.hpp>


#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

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

//! for use with shared_ptr and arrays allocated with new[]
template <typename T>
struct array_deleter {

  void operator()(T const *p) { 

    delete[] p; 
  }
};

VncClient *VncClient::plugin = NULL;

static const std::string config("COVER.Plugin.VncClient");

//! handle update of image region
void VncClient::rfbUpdate(rfbClient* client, int x, int y, int w, int h)
{
   // should not happen too often: we receive rfbTile messages instead
   //std::cerr << "rfb update: " << w << "x" << h << "@+" << x << "+" << y << std::endl;
   //plugin->colorTex->getImage()->dirty();
}

//! handle resize of remote framebuffer
rfbBool VncClient::rfbResize(rfbClient *client)
{
   //std::cerr << "resize: " << client->width << "x" << client->height << std::endl;
   plugin->m_fbImg->allocateImage(client->width, client->height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
   client->frameBuffer = plugin->m_fbImg->data();

   return true;
}

void VncClient::fillMatricesMessage(matricesMsg &msg, int channel, int viewNum, bool second) {

   msg.type = rfbMatrices;
   msg.viewNum = m_channelBase + viewNum;
   msg.time = cover->frameTime();
   const osg::Matrix &t = cover->getXformMat();
   const osg::Matrix &s = cover->getObjectsScale()->getMatrix();
   const osg::Matrix model = s * t;

   const channelStruct &chan = coVRConfig::instance()->channels[channel];
   if (chan.viewportNum < 0)
      return;
   const viewportStruct &vp = coVRConfig::instance()->viewports[chan.viewportNum];
   if (vp.window < 0)
      return;
   const windowStruct &win = coVRConfig::instance()->windows[vp.window];
   msg.width = (vp.viewportXMax - vp.viewportXMin) * win.sx;
   msg.height = (vp.viewportYMax - vp.viewportYMin) * win.sy;

   bool left = chan.stereoMode == osg::DisplaySettings::LEFT_EYE;
   if (second)
      left = true;
   const osg::Matrix &view = left ? chan.leftView : chan.rightView;
   const osg::Matrix &proj = left ? chan.leftProj : chan.rightProj;

#if 0
   std::cerr << "retrieving matrices for channel: " << channel << ", view: " << viewNum << ", second: " << second << ", left: " << left << std::endl;
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
void VncClient::sendMatricesMessage(rfbClient *client, std::vector<matricesMsg> &messages, uint32_t requestNum) {
   
   if (!client)
      return;

   messages.back().last = 1;
   //std::cerr << "requesting " << messages.size() << " views" << std::endl;
   for (size_t i=0; i<messages.size(); ++i) {
      matricesMsg &msg = messages[i];
      msg.requestNumber = requestNum;
      if(!WriteToRFBServer(client, (char *)&msg, sizeof(msg))) {
         rfbClientLog("sendMatricesMessage: write error(%d: %s)", errno, strerror(errno));
      }
   }
}

//! handle RFB matrices message
rfbBool VncClient::rfbMatricesMessage(rfbClient *client, rfbServerToClientMsg *message) {

   if (message->type != rfbMatrices)
      return FALSE;

   rfbClientSetClientData(client, (void *)rfbMatricesMessage, (void *)1);
   std::cerr << "rfbMatrices enabled" << std::endl;

   matricesMsg msg;
   if (!ReadFromRFBServer(client, ((char*)&msg)+1, sizeof(msg)-1))
      return TRUE;

   return TRUE;
}

//! send lighting parameters to server
void VncClient::sendLightsMessage(rfbClient *client) {

   if (!client)
      return;

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

   if (!WriteToRFBServer(client, (char *)&msg, sizeof(msg))) {
      rfbClientLog("sendLightsMessage: write error(%d: %s)", errno, strerror(errno));
   }
}

//! handle RFB lights message
rfbBool VncClient::rfbLightsMessage(rfbClient *client, rfbServerToClientMsg *message) {

   if (message->type != rfbLights)
      return FALSE;

   rfbClientSetClientData(client, (void *)rfbLightsMessage, (void *)1);
   std::cerr << "rfbLights enabled" << std::endl;

   lightsMsg msg;
   if (!ReadFromRFBServer(client, ((char*)&msg)+1, sizeof(msg)-1))
      return TRUE;

   plugin->sendLightsMessage(client);

   return TRUE;
}

//! store results of server bounding sphere query
struct BoundsData {
   BoundsData()
      : updated(false)
   {
   }
   osg::ref_ptr<osg::Node> boundsNode;
   bool updated;
};

//! handle RFB bounds message
rfbBool VncClient::rfbBoundsMessage(rfbClient *client, rfbServerToClientMsg *message) {

   std::cerr << "receiving bounds msg..." << std::endl;

   if (message->type != rfbBounds)
      return FALSE;

   boundsMsg msg;
   if (!ReadFromRFBServer(client, ((char*)&msg)+1, sizeof(msg)-1))
      return TRUE;

   //std::cerr << "center: " << msg.center[0] << " " << msg.center[1] << " " << msg.center[2] << ", radius: " << msg.radius << std::endl;

   BoundsData *bd = static_cast<BoundsData *>(rfbClientGetClientData(client, (void *)rfbBoundsMessage));
   if (!bd) {
      std::cerr << "rfbBounds enabled" << std::endl;
      msg.type = rfbBounds;
      msg.sendreply = 0;
      WriteToRFBServer(client, (char *)&msg, sizeof(msg));

      bd = new BoundsData();
      rfbClientSetClientData(client, (void *)rfbBoundsMessage, (void *)bd);
      bd->boundsNode = new osg::Node;
   }
   bd->updated = true;
   bd->boundsNode->setInitialBound(osg::BoundingSphere(osg::Vec3(msg.center[0], msg.center[1], msg.center[2]), msg.radius));

   osg::BoundingSphere bs = bd->boundsNode->getBound();
   std::cerr << "server bounds: " << bs.center() << ", " << bs.radius() << std::endl;

   return TRUE;
}

//! Task structure for submitting to Intel Threading Building Blocks work //queue
struct DecodeTask: public tbb::task {

   DecodeTask(tbb::concurrent_queue<boost::shared_ptr<tileMsg> > &resultQueue, boost::shared_ptr<tileMsg> msg, boost::shared_ptr<char> payload)
   : resultQueue(resultQueue)
   , msg(msg)
   , payload(payload)
   , rgba(NULL)
   , depth(NULL)
   {}

   tbb::concurrent_queue<boost::shared_ptr<tileMsg> > &resultQueue;
   boost::shared_ptr<tileMsg> msg;
   boost::shared_ptr<char> payload;
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

      boost::shared_ptr<char> decompbuf = payload;
      if (msg->compression & rfbTileSnappy) {
#ifdef HAVE_SNAPPY
         decompbuf.reset(new char[sz], array_deleter<char>());
         snappy::RawUncompress(payload.get(), msg->size, decompbuf.get());
#else
         resultQueue.push(msg);
         return NULL;
#endif
      }

      if (msg->format!=rfbColorRGBA && (msg->compression & rfbTileDepthQuantize)) {

         depthdequant(depth, decompbuf.get(), bpp==4 ? DepthFloat : DepthInteger, bpp, msg->x, msg->y, msg->width, msg->height, msg->totalwidth);
      } else if (msg->compression == rfbTileJpeg) {

#ifdef HAVE_TURBOJPEG
         TjContext::reference tj = tjContexts.local();
         char *dest = msg->format==rfbColorRGBA ? rgba : depth;
         int w=-1, h=-1;
         tjDecompressHeader(tj.handle, reinterpret_cast<unsigned char *>(payload.get()), msg->size, &w, &h);
         dest += (msg->y*msg->totalwidth+msg->x)*bpp;
         int ret = tjDecompress(tj.handle, reinterpret_cast<unsigned char *>(payload.get()), msg->size, reinterpret_cast<unsigned char *>(dest), msg->width, msg->totalwidth*bpp, msg->height, bpp, TJPF_BGR);
         if (ret == -1)
            std::cerr << "VncClient: JPEG error: " << tjGetErrorStr() << std::endl;
#else
         std::cerr << "VncClient: no JPEG support" << std::endl;
#endif
      } else {

         char *dest = msg->format==rfbColorRGBA ? rgba : depth;
         for (int yy=0; yy<msg->height; ++yy) {
            memcpy(dest+((msg->y+yy)*msg->totalwidth+msg->x)*bpp, decompbuf.get()+msg->width*yy*bpp, msg->width*bpp);
         }
      }

      resultQueue.push(msg);
      return NULL;
   }
};

// returns true if a frame has been discarded - should be called again
bool VncClient::updateTileQueue() {

   //std::cerr << "tiles: " << m_queued << " queued, " << m_deferred.size() << " deferred" << std::endl;
   boost::shared_ptr<tileMsg> msg;
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

void VncClient::finishFrame(const tileMsg &msg) {

   m_waitForDecode = false;

   ++m_remoteFrames;
   double delay = cover->frameTime() - msg.requestTime;
   m_accumDelay += delay;
   if (m_minDelay > delay)
      m_minDelay = delay;
   if (m_maxDelay < delay)
      m_maxDelay = delay;

   m_depthBytesS += m_depthBytes;
   m_rgbBytesS += m_rgbBytes;
   m_depthBppS = m_depthBpp;
   m_numPixelsS = m_numPixels;

   //std::cerr << "finishFrame: t=" << msg.timestep << ", req=" << m_requestedTimestep << std::endl;
   if (m_remoteTimestep == m_requestedTimestep) {
      commitTimestep(m_remoteTimestep);
      m_requestedTimestep = -1;
      m_frameReady = false;
   } else {
      m_frameReady = true;
   }
}

void VncClient::checkSwapFrame() {

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
   doSwap = coVRMSController::instance()->syncBool(doSwap);
   if (doSwap) {
      swapFrame();
   }
}

void VncClient::swapFrame() {

   assert(m_frameReady = true);
   m_frameReady = false;

   m_visibleTimestep = m_remoteTimestep;
   m_remoteTimestep = -1;

   //std::cerr << "frame: timestep=" << m_visibleTimestep << std::endl;

   m_drawer->swapFrame();
}

void VncClient::handleTileMeta(const tileMsg &msg) {

#ifdef CONNDEBUG
   bool first = msg.flags&rfbTileFirst;
   bool last = msg.flags&rfbTileLast;
   if (first || last) {
      std::cout << "TILE: " << (first?"F":" ") << "." << (last?"L":" ") << "req: " << msg.requestNumber << ", view: " << msg.viewNum << ", frame: " << msg.frameNumber << ", dt: " << cover->frameTime() - msg.requestTime  << std::endl;
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

   m_remoteTimestep = msg.timestep;
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

bool VncClient::canEnqueue() const {

   return !plugin->m_waitForDecode && plugin->m_deferredFrames==0;
}

void VncClient::enqueueTask(DecodeTask *task) {

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

//! handle RFB image tile message
rfbBool VncClient::rfbTileMessage(rfbClient *client, rfbServerToClientMsg *message) {

   if (message->type != rfbTile)
      return FALSE;

   if (!rfbClientGetClientData(client, (void *)rfbTileMessage)) {
      rfbClientSetClientData(client, (void *)rfbTileMessage, (void *)1);
      std::cerr << "rfbTile enabled" << std::endl;

      tileMsg msg;
      msg.compression = rfbTileRaw;
      msg.compression |= rfbTileDepthQuantize;
#ifdef HAVE_TURBOJPEG
      msg.compression |= rfbTileJpeg;
#endif
#ifdef HAVE_SNAPPY
      msg.compression |= rfbTileSnappy;
#endif
      msg.size = 0;
      WriteToRFBServer(client, (char *)&msg, sizeof(msg));
   }


   boost::shared_ptr<tileMsg> msg(new tileMsg);
   if (!ReadFromRFBServer(client, ((char*)(&*msg))+1, sizeof(*msg)-1)) {
      return TRUE;
   }
   boost::shared_ptr<char> payload;
#ifdef CONNDEBUG
   int flags = msg->flags;
   bool first = flags&rfbTileFirst;
   bool last = flags&rfbTileLast;
   std::cerr << "rfbTileMessage: req: " << msg->requestNumber << ", first: " << first << ", last: " << last << std::endl;
#endif
   if (msg->size > 0) {
      payload.reset(new char[msg->size], array_deleter<char>());
      if (!ReadFromRFBServer(client, &*payload, msg->size)) {
         return TRUE;
      }
   }

   if (msg->size==0 && msg->width==0 && msg->height==0 && msg->flags == 0) {
      std::cerr << "received initial tile - ignoring" << std::endl;
      return TRUE;
   }

   if (msg->flags & rfbTileLast)
      plugin->m_lastTileAt = plugin->m_receivedTiles.size();
   plugin->m_receivedTiles.push_back(TileMessage(msg, payload));

   return TRUE;
}

bool VncClient::handleTileMessage(boost::shared_ptr<tileMsg> msg, boost::shared_ptr<char> payload) {

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

//! send RFB application message to server
void VncClient::sendApplicationMessage(rfbClient *client, int type, int length, const void *data) {

   if (!client)
      return;

   applicationMsg msg;
   msg.type = rfbApplication;
   msg.appType = type;
   msg.version = 0;
   msg.sendreply = 0;
   msg.size = length;

   if(!WriteToRFBServer(client, (char *)&msg, sizeof(msg))) {
      rfbClientLog("sendApplicationMessage: write error(%d: %s)", errno, strerror(errno));
   }
   if(!WriteToRFBServer(client, (char *)data, length)) {
      rfbClientLog("sendApplicationMessage: write data error(%d: %s)", errno, strerror(errno));
   }

}

//! handle RFB application message
rfbBool VncClient::rfbApplicationMessage(rfbClient *client, rfbServerToClientMsg *message) {

   if (message->type != rfbApplication)
      return FALSE;

   if (!rfbClientGetClientData(client, (void *)rfbApplicationMessage)) {
      rfbClientSetClientData(client, (void *)rfbApplicationMessage, (void *)1);
      std::cerr << "rfbApplication enabled" << std::endl;

      applicationMsg msg;
      msg.type = rfbApplication;
      msg.appType = 0;
      msg.sendreply = 0;
      msg.version = 0;
      msg.size = 0;
      WriteToRFBServer(client, (char *)&msg, sizeof(msg));
   }

   applicationMsg msg;
   if (!ReadFromRFBServer(client, ((char*)&msg)+1, sizeof(msg)-1))
      return TRUE;

   std::vector<char> buf(msg.size);
   if (!ReadFromRFBServer(client, &buf[0], msg.size)) {
         return TRUE;
   }

   switch(msg.appType) {
      case rfbAddObject:
      {
         appAddObject app;
         memcpy(&app, &buf[0], sizeof(app));
         size_t idx = sizeof(app);

#define READ(n, l) \
         std::string n(&buf[idx], l); \
         idx += l

         READ(objName, app.namelen);
         RemoteRenderObject *ro = new RemoteRenderObject(objName.c_str());
         plugin->m_objectMap[objName] = ro;
         ro->ref();

         if (app.geonamelen) {
            READ(name, app.geonamelen);
            ro->setGeometry(plugin->findObject(name));
         }
         if (app.normnamelen) {
            READ(name, app.normnamelen);
            ro->setNormals(plugin->findObject(name));
         }
         if (app.colnamelen) {
            READ(name, app.colnamelen);
            ro->setColors(plugin->findObject(name));
         }
         if (app.texnamelen) {
            READ(name, app.texnamelen);
            ro->setTexture(plugin->findObject(name));
         }

         for (size_t i=0; i<app.nattrib; ++i) {

            appAttribute attr;
            memcpy(&attr, &buf[idx], sizeof(attr));
            idx += sizeof(attr);

            READ(name, attr.namelen);
            READ(value, attr.valuelen);

            ro->addAttribute(name, value);
         }

#undef READ

         if (const char *pluginName=ro->getAttribute("MODULE")) {
            cover->addPlugin(pluginName);
         }

         if (app.isbase) {
            RenderObject *objs[4] = {
               ro->getGeometry(),
               ro->getNormals(),
               ro->getColors(),
               ro->getTexture()
            };
            for (int i=0; i<4; ++i) {
               if (!objs[i])
                  continue;

               const char *inter = objs[i]->getAttribute("INTERACTOR");
               if (inter) {
                  //std::cerr << "got interactor: " << inter << std::endl;
                  RenderObject *geo = ro->getGeometry();
                  if (!geo)
                     geo = ro;
                  coInteractor *it= new coRemoteCoviseInteractor(plugin, geo->getName(), geo, inter);
                  if (it->getPluginName())
                     cover->addPlugin(it->getPluginName());

                  it->incRefCount();
                  coVRPluginList::instance()->newInteractor(ro, it);
                  it->decRefCount();
               }
            }

            coVRPluginList::instance()->addObject(ro, ro->getGeometry(), ro->getNormals(), ro->getColors(), ro->getTexture(),
                  NULL,
                  0, Bind::None, Pack::None, NULL, NULL, NULL, NULL, 
                  0, Bind::None, NULL, NULL, NULL,
                  0.);
         }
      }
      break;

      case rfbRemoveObject:
      {
         appRemoveObject app;
         memcpy(&app, &buf[0], sizeof(app));
         size_t idx = sizeof(app);
         std::string name(&buf[idx], app.namelen);
         idx += app.namelen;

         coVRPluginList::instance()->removeObject(name.c_str(), app.replace);

         ObjectMap::iterator it = plugin->m_objectMap.find(name);
         if (it != plugin->m_objectMap.end()) {
            RemoteRenderObject *ro = it->second;
            plugin->m_objectMap.erase(it);
            RenderObject *objs[4] = {
               ro->getGeometry(),
               ro->getNormals(),
               ro->getColors(),
               ro->getTexture()
            };

            for (int i=0; i<4; ++i) {
               if (!objs[i])
                  continue;
               if (!objs[i]->getName())
                  continue;
               ObjectMap::iterator it2 = plugin->m_objectMap.find(objs[i]->getName());
               if (it2 == plugin->m_objectMap.end())
                  continue;
               it2->second->unref();
               plugin->m_objectMap.erase(it2);
            }

            ro->unref();
         }
      }
      break;

      case rfbAnimationTimestep: {
         appAnimationTimestep app;
         memcpy(&app, &buf[0], sizeof(app));
         plugin->m_numRemoteTimesteps = app.total;
      }
      break;
   }

   return TRUE;
}

//! find RemoteRenderObject corresponding to name
RemoteRenderObject *VncClient::findObject(const std::string &name) const {

   ObjectMap::const_iterator it = m_objectMap.find(name);
   if (it == m_objectMap.end()) {
      std::cerr << "obj not found: " << name << std::endl;
      return NULL;
   }

   return it->second;
}

// RFB client protocol extensions
static rfbClientProtocolExtension matricesExt = {
   matricesEncodings, // encodings
   NULL, // handleEncoding
   VncClient::rfbMatricesMessage, // handleMessage
   NULL, // next extension
};

static rfbClientProtocolExtension lightsExt = {
   lightsEncodings, // encodings
   NULL, // handleEncoding
   VncClient::rfbLightsMessage, // handleMessage
   NULL, // next extension
};

static rfbClientProtocolExtension boundsExt = {
   boundsEncodings, // encodings
   NULL, // handleEncoding
   VncClient::rfbBoundsMessage, // handleMessage
   NULL, // next extension
};

static rfbClientProtocolExtension tileExt = {
   tileEncodings, // encodings
   NULL, // handleEncoding
   VncClient::rfbTileMessage, // handleMessage
   NULL, // next extension
};

static rfbClientProtocolExtension applicationExt = {
   applicationEncodings, // encodings
   NULL, // handleEncoding
   VncClient::rfbApplicationMessage, // handleMessage
   NULL, // next extension
};

//! called when plugin is loaded
VncClient::VncClient()
: m_requestedTimestep(-1)
, m_remoteTimestep(-1)
, m_visibleTimestep(-1)
, m_numRemoteTimesteps(-1)
{
   assert(plugin == NULL);
   plugin = this;
   //fprintf(stderr, "new VncClient plugin\n");
}
//! called after plug-in is loaded and scenegraph is initialized
bool VncClient::init()
{
   m_client = NULL;
   m_haveConnection = false;
   m_listen = false;

   m_reproject = true;
   m_adapt = true;
   m_frameReady = false;

#if 0
   std::cout << "COVER waiting for debugger: pid=" << getpid() << std::endl;
   sleep(10);
   std::cout << "COVER continuing..." << std::endl;
#endif

   m_fbImg = new osg::Image();

   rfbClientRegisterExtension(&matricesExt);
   rfbClientRegisterExtension(&lightsExt);
   rfbClientRegisterExtension(&boundsExt);
   rfbClientRegisterExtension(&tileExt);
   rfbClientRegisterExtension(&applicationExt);

   //SendFramebufferUpdateRequest(m_client, 0, 0, m_client->width, m_client->height, FALSE);

   m_benchmark = covise::coCoviseConfig::isOn("benchmark", config, true);
   m_compress = covise::coCoviseConfig::getInt("vncCompress", config, 0);
   if (m_compress < 0) m_compress = 0;
   if (m_compress > 9) m_compress = 9;
   m_quality = covise::coCoviseConfig::getInt("vncQuality", config, 9);
   if (m_quality < 0) m_quality = 0;
   if (m_quality > 9) m_quality = 9;
   m_lastStat = cover->currentTime();
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
   m_drawer->switchReprojection(m_reproject);
   m_drawer->switchAdaptivePointSize(m_adapt);

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

   return true;
}

//! this is called if the plugin is removed at runtime
VncClient::~VncClient()
{
   cover->getScene()->removeChild(m_drawer);

   delete m_menu;
   delete m_menuItem;

   clientCleanup(m_client);
   plugin = NULL;
}

void VncClient::menuEvent(coMenuItem *item) {

   if (item == m_reprojCheck) {
      m_reproject = m_reprojCheck->getState();
      m_drawer->switchReprojection(m_reproject);
   }
   if (item == m_adaptCheck) {
       m_adapt = m_adaptCheck->getState();
       m_drawer->switchAdaptivePointSize(m_adapt);
   }
   if (item == m_allTilesCheck) {
   }
}

//! this is called before every frame, used for polling for RFB messages
void
VncClient::preFrame()
{
   static double lastUpdate = -DBL_MAX;
   static double lastMatrices = -DBL_MAX;
   static double lastConnectionTry = -DBL_MAX;
   static int remoteSkipped = 0;

   if (coVRMSController::instance()->isMaster()) {
      if (cover->frameTime() - lastConnectionTry > 3.) {
         lastConnectionTry = cover->frameTime();
         connectClient();
      }
   }

   bool connected = coVRMSController::instance()->syncBool(m_client && !m_listen);
   if (m_haveConnection != connected) {
       if (connected)
           cover->getScene()->addChild(m_drawer);
       else
           cover->getScene()->removeChild(m_drawer);
       m_haveConnection = connected;
   }

   if (!connected)
      return;

   coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
   if (m_numRemoteTimesteps > 0)
      coVRAnimationManager::instance()->setNumTimesteps(m_numRemoteTimesteps, this);
   else
      coVRAnimationManager::instance()->removeTimestepProvider(this);

   osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[0].window;
   int x,y,w,h;
   win->getWindowRectangle(x, y, w, h);
   if (activeConfig.nearValue != coVRConfig::instance()->nearClip()
         || activeConfig.farValue != coVRConfig::instance()->farClip()
         || activeConfig.width != w
         || activeConfig.height != h) {
      activeConfig.nearValue = coVRConfig::instance()->nearClip();
      activeConfig.farValue = coVRConfig::instance()->farClip();
      activeConfig.width = w;
      activeConfig.height = h;

      activeConfig.vsize = coVRConfig::instance()->screens[0].vsize;
      activeConfig.hsize = coVRConfig::instance()->screens[0].hsize;
      activeConfig.screenPos[0] = coVRConfig::instance()->screens[0].xyz[0];
      activeConfig.screenPos[1] = coVRConfig::instance()->screens[0].xyz[1];
      activeConfig.screenPos[2] = coVRConfig::instance()->screens[0].xyz[2];
      activeConfig.screenRot[0] = coVRConfig::instance()->screens[0].hpr[0];
      activeConfig.screenRot[1] = coVRConfig::instance()->screens[0].hpr[1];
      activeConfig.screenRot[2] = coVRConfig::instance()->screens[0].hpr[2];

      sendApplicationMessage(m_client, rfbScreenConfig, sizeof(activeConfig), &activeConfig);
   }

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

   if (coVRMSController::instance()->isMaster()) {
      bool haveMessage = false;
      size_t oldFrame = m_remoteFrames;
      while (handleRfbMessages() > 0) {
         haveMessage = true;
         lastUpdate = cover->frameTime();

         if (m_remoteFrames != oldFrame)
            break;
      }

      if ((haveMessage || cover->frameTime()-lastMatrices>0.03) && m_client && rfbClientGetClientData(m_client, (void *)rfbMatricesMessage)) {
         sendLightsMessage(m_client);
         sendMatricesMessage(m_client, messages, m_matrixNum++);
         lastMatrices = cover->frameTime();
      }
   }

   int ntiles = m_receivedTiles.size();
   if (m_lastTileAt >= 0) {
      ntiles = m_lastTileAt+1;
      m_lastTileAt = -1;
   } else if (ntiles > 50) {
      ntiles = 50;
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
   coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
   if (coVRMSController::instance()->isMaster()) {
      for (int i=0; i<ntiles; ++i) {
         TileMessage &tile = m_receivedTiles.front();
         if (broadcastTiles) {
            coVRMSController::instance()->sendSlaves(tile.msg.get(), sizeof(*tile.msg));
            if (tile.msg->size > 0) {
               coVRMSController::instance()->sendSlaves(tile.payload.get(), tile.msg->size);
            }
         } else {
            int channelBase = coVRConfig::instance()->numChannels();
            for (int s=0; s<coVRMSController::instance()->getNumSlaves(); ++s) {
               int numChannels = m_numChannels[i];
               size_t sz = tile.msg->size;
               if (tile.msg->viewNum < channelBase || tile.msg->viewNum >= channelBase+numChannels) {
                  //tile.msg->size = 0;
               }
               coVRMSController::instance()->sendSlave(s, tile.msg.get(), sizeof(*tile.msg));
               if (tile.msg->size > 0) {
                  coVRMSController::instance()->sendSlave(s, tile.payload.get(), tile.msg->size);
               }
               tile.msg->size = sz;
               channelBase += numChannels;
            }
         }
         handleTileMessage(tile.msg, tile.payload);
         m_receivedTiles.pop_front();
      }
      //assert(m_receivedTiles.empty());
   } else {
      for (int i=0; i<ntiles; ++i) {
         boost::shared_ptr<tileMsg> msg(new tileMsg);
         coVRMSController::instance()->readMaster(msg.get(), sizeof(*msg));
         boost::shared_ptr<char> payload;
         if (msg->size > 0) {
            payload.reset(new char[msg->size], array_deleter<char>());
            coVRMSController::instance()->readMaster(payload.get(), msg->size);
         }
         handleTileMessage(msg, payload);
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

   checkSwapFrame();

   ++m_localFrames;
   double diff = cover->frameTime() - m_lastStat;
   if (plugin->m_benchmark && diff > 3.) {

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

//! called when scene bounding sphere is required
void VncClient::expandBoundingSphere(osg::BoundingSphere &bs) {

   if (!m_client)
      return;

   BoundsData *bd = static_cast<BoundsData *>(rfbClientGetClientData(m_client, (void *)rfbBoundsMessage));
   if (!bd)
      return;

   boundsMsg msg;
   msg.type = rfbBounds;
   msg.sendreply = 1;
   WriteToRFBServer(m_client, (char *)&msg, sizeof(msg));

   double start = cover->currentTime();
   bd->updated = false;
   do {
      if (handleRfbMessages() < 0)
         return;
      double elapsed = cover->currentTime() - start;
      if (elapsed > 2.) {
         start = cover->currentTime();
#if 0
         std::cerr << "VncClient: still waiting for bounding sphere from server" << std::endl;
#else
         std::cerr << "VncClient: re-requesting bounding sphere from server" << std::endl;

         boundsMsg msg;
         msg.type = rfbBounds;
         msg.sendreply = 1;
         WriteToRFBServer(m_client, (char *)&msg, sizeof(msg));
#endif
      }
   }
   while (!bd->updated);

   bs.expandBy(bd->boundsNode->getBound());
}

void VncClient::setTimestep(int t) {

   m_frameReady = true;
   checkSwapFrame();
}

void VncClient::requestTimestep(int t) {

   //std::cerr << "m_requestedTimestep: " << m_requestedTimestep << " -> " << t << std::endl;
   if (t < 0)
      t = 0;

   if (m_remoteTimestep==t || (m_remoteTimestep==-1 && m_visibleTimestep==t)) {
      commitTimestep(t);
   } else {
      m_requestedTimestep = t;

      appAnimationTimestep msg;
      msg.current = t;
      msg.total = coVRAnimationManager::instance()->getNumTimesteps();
      sendApplicationMessage(m_client, rfbAnimationTimestep, sizeof(msg), &msg);
   }
}

//! returns number of handled messages, -1 on error
int VncClient::handleRfbMessages()
{
   if (!m_client || m_listen)
      return -1;

   const int n = WaitForMessage(m_client, 1);
   if (n<0) {
      clientCleanup(m_client);
      return n;
   }
   if (n==0) {
      return n;
   }
   //std::cerr << n << " messages in queue" << std::endl;
   if(!HandleRFBServerMessage(m_client)) {
      if (n > 0)
         clientCleanup(m_client);
      return n;
   }

   return n;
}

bool VncClient::connectClient() {

   if (m_client && !m_listen)
      return true;

   std::string host = covise::coCoviseConfig::getEntry("rfbHost", config, "localhost");
   int port = covise::coCoviseConfig::getInt("rfbPort", config, 5900);

   if (!m_client) {
      m_client = rfbGetClient(8, 3, 4);
      m_client->GotFrameBufferUpdate = rfbUpdate;
      m_client->MallocFrameBuffer = rfbResize;
      m_client->canHandleNewFBSize = TRUE;
      m_client->serverHost = NULL;

      m_client->appData.compressLevel = m_compress;
      m_client->appData.qualityLevel = m_quality;

      memset(&activeConfig, 0, sizeof(activeConfig)); // force sending of window size

      if (covise::coCoviseConfig::isOn("listen", config, false)) {

         m_listen = true;
         //m_client->listenAddress = strdup(covise::coCoviseConfig::getEntry("rfbHost", config, "localhost").c_str());
         m_client->listenPort = covise::coCoviseConfig::getInt("listenPort", config, 31059);
      } else {

         m_client->serverHost = strdup(host.c_str());
         m_client->serverPort = port;
      }
   }

   bool reverse = false;
   if (m_listen) {
      int ret = listenForIncomingConnectionsNoFork(m_client, 0);
      if (ret < 0) {
         // error
         rfbClientCleanup(m_client);
         m_listen = false;
         m_client = NULL;
         return false;
      } else if (ret == 0) {
         // timeout - try again on same client
         return false;
      } else if (ret > 0) {
         std::cerr << "VncClient: server connected on port " << m_client->listenPort << std::endl;
         m_listen = false;
         reverse = true;
      }
   }

   if (!rfbInitClient(m_client, NULL, NULL)) {
      if (reverse) {
         std::cerr << "VncClient: client initialisation failed - still listening on port " << m_client->listenPort  << std::endl;
      } else {
         std::cerr << "VncClient: connection to " << host << ":" << port << " failed" << std::endl;
      }
      //FIXME: crash
      //rfbClientCleanup(m_client);
      m_client = NULL;
      return false;
   }

   SendFramebufferUpdateRequest(m_client, 0, 0, m_client->width, m_client->height, FALSE);
   std::cerr << "RFB client: compress=" << m_client->appData.compressLevel << ", quality=" << m_client->appData.qualityLevel << std::endl;

   return true;
}

//! clean up when connection to server is lost
void VncClient::clientCleanup(rfbClient *cl) {

   if (!cl)
      return;

   std::cerr << "disconnected from server" << std::endl;

   for (ObjectMap::iterator it = m_objectMap.begin();
         it != m_objectMap.end();
         ++it) {
      it->second->unref();
   }
   m_objectMap.clear();

   rfbClientCleanup(cl);
   if (m_client == cl) {
      m_client = NULL;
      m_numRemoteTimesteps = -1;
   }
}

//! forward interactor feedback to server
void VncClient::sendFeedback(const char *info, const char *key, const char *data)
{
   appFeedback app;
   app.infolen = 0;
   app.keylen = 0;
   app.datalen = 0;

   std::vector<char> buf;
   std::back_insert_iterator<std::vector<char> > back_inserter(buf);
   std::copy((char *)&app, ((char *)&app)+sizeof(app), back_inserter);
   if (info) {
      app.infolen = strlen(info);
      std::copy(info, info+app.infolen, back_inserter);
   }
   if (key) {
      app.keylen = strlen(key);
      std::copy(key, key+app.keylen, back_inserter);
   }
   if (data) {
      app.datalen = strlen(data);
      std::copy(data, data+app.datalen, back_inserter);
   }
   memcpy(&buf[0], &app, sizeof(app));

   sendApplicationMessage(m_client, rfbFeedback, buf.size(), &buf[0]);
}

COVERPLUGIN(VncClient)
