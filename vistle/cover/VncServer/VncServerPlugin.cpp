/**\file
 * \brief VncServerPlugin plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014 HLRS
 *
 * \copyright LGPL2+
 */

#include <config/CoviseConfig.h>

#include <kernel/coVRConfig.h>
#include <kernel/OpenCOVER.h>
#include <kernel/coVRMSController.h>
#include <kernel/coVRPluginSupport.h>
#include <kernel/VRViewer.h>
#include <kernel/VRSceneGraph.h>
#include <kernel/RenderObject.h>
#include <kernel/coVRLighting.h>
#include <kernel/input/input.h>
#include <kernel/input/coMousePointer.h>

#include <PluginUtil/PluginMessageTypes.h>

#include <osg/GLExtensions>
#include <osg/MatrixTransform>
#include <osgGA/GUIEventAdapter>
#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>
#include <OpenThreads/Condition>

#include <limits>

#undef HAVE_CUDA

#include "VncServerPlugin.h"

#ifdef HAVE_CUDA
#include <cuda_runtime_api.h>
#endif

#ifdef HAVE_COVISE
#include <appl/RenderInterface.h>
#endif

#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

#include <rfb/rfb.h>

#include <rhr/ReadBackCuda.h>
#include <rhr/rfbext.h>
#include <rhr/vncserver.h>
#include <rhr/depthquant.h>

#ifndef GL_DEPTH_STENCIL_TO_RGBA_NV
#define GL_DEPTH_STENCIL_TO_RGBA_NV 0x886E
#endif
#ifndef GL_DEPTH_STENCIL_TO_BGRA_NV
#define GL_DEPTH_STENCIL_TO_BGRA_NV 0x886F
#endif

VncServerPlugin *VncServerPlugin::plugin = NULL;

//! per-client data: supported extensions
struct ClientData {
   ClientData()
      : supportsBounds(false)
      , supportsDepth(false)
      , depthCompressions(0)
      , supportsApplication(false)
   {
   }
   bool supportsBounds;
   bool supportsDepth;
   uint8_t depthCompressions;
   std::vector<char> buf;
   bool supportsApplication;
};

//! called when plugin is loaded
VncServerPlugin::VncServerPlugin()
{
   assert(plugin == NULL);
   plugin = this;
   //fprintf(stderr, "new VncServer plugin\n");
}


//! called after plug-in is loaded and scenegraph is initialized
bool VncServerPlugin::init()
{
   m_vnc = NULL;

   m_delay = 0;
   m_lastMatrixTime = 0.;
   m_cudaColor = NULL;
   m_cudaDepth = NULL;
   m_cudapinnedmemory = false;
   m_glextChecked = false;
   std::string config("COVER.Plugin.VncServer");

   osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[0].window;
   if (!win)
      return false;

   int x,y,w,h;
   win->getWindowRectangle(x,y,w,h);
   m_width = w;
   m_height = h;

   m_benchmark = covise::coCoviseConfig::isOn("benchmark", config, false);
   m_errormetric = covise::coCoviseConfig::isOn("errormetric", config, false);
   m_compressionrate = covise::coCoviseConfig::isOn("compressionrate", config, false);

#ifdef HAVE_CUDA
   if(covise::coCoviseConfig::isOn("cudaread", config, true))
   {
      m_cudaColor = new ReadBackCuda();
      m_cudaDepth = new ReadBackCuda();
   }
   m_cudapinnedmemory = covise::coCoviseConfig::isOn("cudapinned", config, true);
#else
   m_cudapinnedmemory = false;
#endif

   m_depthprecision = covise::coCoviseConfig::getInt("depthPrecision", config, 32);
   m_depthfloat = covise::coCoviseConfig::isOn("depthFloat", config, false);
   m_depthquant = covise::coCoviseConfig::isOn("depthQuant", config, true);
   m_depthcopy = covise::coCoviseConfig::isOn("depthCopy", config, true);

   if (coVRMSController::instance()->isMaster()) {
      m_vnc = new VncServer(m_width, m_height, coCoviseConfig::getInt("rfbPort", config, 5900));
   }

   std::string host = covise::coCoviseConfig::getEntry("clientHost", config);
   if(!host.empty())
   {
      Client c;
      c.host = host;
      c.port = covise::coCoviseConfig::getInt("clientPort", config, 31042);
      m_clientList.push_back(c);
   }
   host = covise::coCoviseConfig::getEntry("alternateClientHost", config);
   if(!host.empty())
   {
      Client c;
      c.host = host;
      c.port = covise::coCoviseConfig::getInt("alternateClientPort", config, 31042);
      m_clientList.push_back(c);
   }

   return true;
}


// this is called if the plugin is removed at runtime
VncServerPlugin::~VncServerPlugin()
{
   delete m_vnc;
   if (m_cudapinnedmemory) {
#ifdef HAVE_CUDA
      //cudaFreeHost(m_screen->frameBuffer);
#endif
#if 0
   } else {
      delete[] m_screen->frameBuffer;
#endif
   }

   delete m_cudaColor;
   delete m_cudaDepth;

   //fprintf(stderr,"VncServerPlugin::~VncServerPlugin\n");
}

//! handle matrix update message
rfbBool VncServerPlugin::handleMatricesMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbMatrices)
      return FALSE;

   matricesMsg msg;

   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleMatricesMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }

   plugin->m_lastMatrixTime = msg.time;

   osg::Matrix view, transform, scale;
   for (int i=0; i<16; ++i) {
      view.ptr()[i] = msg.viewer[i];
      transform.ptr()[i] = msg.transform[i];
      scale.ptr()[i] = msg.scale[i];
   }
   VRViewer::instance()->updateViewerMat(view);
   cover->setXformMat(transform);
   cover->getObjectsScale()->setMatrix(scale);

   return TRUE;
}

//! handle light update message
rfbBool VncServerPlugin::handleLightsMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbLights)
      return FALSE;

   lightsMsg msg;

   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleLightsMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }

   for (int i=0; i<lightsMsg::NumLights; ++i) {

      osg::Light *light = NULL;
      if (i == 0)
         light = coVRLighting::instance()->headlight->getLight();
      else if (i == 1)
         light = coVRLighting::instance()->spotlight->getLight();
      else if (i == 2)
         light = coVRLighting::instance()->light1->getLight();
      else if (i == 3)
         light = coVRLighting::instance()->light2->getLight();

      if (!light) {
         continue;
      }

#define SET_VEC(d, setter, src) \
      do { \
         osg::Vec##d v; \
         for (int k=0; k<d; ++k) { \
            v[k] = (src)[k]; \
         } \
         (setter)(v); \
      } while(false);

      SET_VEC(4, light->setPosition, msg.lights[i].position)
      SET_VEC(4, light->setAmbient, msg.lights[i].ambient)
      SET_VEC(4, light->setDiffuse, msg.lights[i].diffuse)
      SET_VEC(4, light->setSpecular, msg.lights[i].specular)
      SET_VEC(3, light->setDirection, msg.lights[i].spot_direction)

#undef SET_VEC

      light->setConstantAttenuation(msg.lights[i].attenuation[0]);
      light->setLinearAttenuation(msg.lights[i].attenuation[1]);
      light->setQuadraticAttenuation(msg.lights[i].attenuation[2]);
      light->setSpotCutoff(msg.lights[i].spot_cutoff);
      light->setSpotExponent(msg.lights[i].spot_exponent);
   }

   return TRUE;
}

#if 0
//! send depth buffer to a client
void VncServerPlugin::sendDepthMessage(rfbClientPtr cl) {

   if (!plugin->m_screen->frameBuffer)
      return;

   depthMsg msg;
   msg.type = rfbDepth;
   msg.requesttime = plugin->m_lastMatrixTime;
#if 0
   const osg::Matrix &v = cover->getViewerMat();
   const osg::Matrix &t = cover->getXformMat();
   const osg::Matrix &s = cover->getObjectsScale()->getMatrix();
   for (int i=0; i<16; ++i) {
      //TODO: swap to big-endian
      msg.requestview[i] = v(i%4, i/4);
      msg.requesttransform[i] = t(i%4, i/4);
      msg.requestscale[i] = s(i%4, i/4);
   }
#else
   const osg::Matrix &view = coVRConfig::instance()->screens[0].rightView;
   const osg::Matrix &proj = coVRConfig::instance()->screens[0].rightProj;
   const osg::Matrix &t = cover->getXformMat();
   const osg::Matrix &s = cover->getObjectsScale()->getMatrix();
   for (int i=0; i<16; ++i) {
      //TODO: swap to big-endian
      msg.view[i] = view(i%4, i/4);
      msg.proj[i] = proj(i%4, i/4);
      msg.transform[i] = t(i%4, i/4);
      msg.scale[i] = s(i%4, i/4);
   }
#endif
   msg.sendreply = 0;
   msg.width = plugin->m_width;
   msg.height = plugin->m_height;
   msg.x = 0;
   msg.y = 0;
   size_t sz = msg.width * msg.height;
   if (plugin->m_depthfloat) {
      msg.format = rfbDepthFloat;
      sz *= 4;
   } else {
      sz *= plugin->m_depthprecision==24 ? 4 : plugin->m_depthprecision/8;
      switch (plugin->m_depthprecision) {
         case 8: msg.format = rfbDepth8Bit; break;
         case 16: msg.format = rfbDepth16Bit; break;
         case 24: msg.format = rfbDepth24Bit; break;
         case 32: msg.format = rfbDepth32Bit; break;
      }
   }
   msg.compression = rfbDepthRaw;

   const char *zbuf = plugin->m_screen->frameBuffer+plugin->m_width*plugin->m_height*4;
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (plugin->m_depthquant && plugin->m_cudaDepth && cd->depthCompressions&rfbDepthQuantize) {
      msg.compression |= rfbDepthQuantize;
      sz = depthquant_size(plugin->m_depthfloat?DepthFloat:DepthInteger, plugin->m_depthprecision<=16?2:3, msg.width, msg.height);
   }
   msg.size = sz;
   double snappyduration = 0.;
#ifdef HAVE_SNAPPY
   if((cd->depthCompressions & rfbDepthSnappy)) {
      if (snappy::MaxCompressedLength(sz) > cd->buf.size()) {
         cd->buf.resize(snappy::MaxCompressedLength(sz));
      }
      double snappystart = 0.;
      if (plugin->m_benchmark)
         snappystart = cover->currentTime();
      size_t compressed = 0;
      snappy::RawCompress(zbuf, sz, &cd->buf[0], &compressed);
      if (plugin->m_benchmark)
         snappyduration = cover->currentTime() - snappystart;
      //std::cerr << "compressed " << sz << " to " << compressed << " (buf: " << cd->buf.size() << ")" << std::endl;
      msg.compression |= rfbDepthSnappy;
      msg.size = compressed;
      if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("sendDepthMessage: write");
      }
      if (rfbWriteExact(cl, &cd->buf[0], msg.size) < 0) {
         rfbLogPerror("sendDepthMessage: write");
      }
   } else
#endif
   {
      if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("sendDepthMessage: write");
      }
      if (rfbWriteExact(cl, zbuf, msg.size) < 0) {
         rfbLogPerror("sendDepthMessage: write");
      }
   }

   if (plugin->m_benchmark || plugin->m_compressionrate) {
      fprintf(stderr, "Depth: raw=%ld, quant=%ld, snappy=%ld, %f s, %f gb/s\n",
            (long)msg.width*msg.height*plugin->m_depthprecision/8,
            (long)sz,
            (long)msg.size,
            snappyduration,
            snappyduration > 0. ? sz/snappyduration/1024/1024/1024 : 0.);
   }
}

//! handle depth request by client
rfbBool VncServerPlugin::handleDepthMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbDepth)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;
   }
   depthMsg msg;
   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleDepthMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }
   cd->depthCompressions = msg.compression;
   size_t sz = msg.width*msg.height;
   switch (msg.format) {
      case rfbDepth8Bit: break;
      case rfbDepth16Bit: sz *= 2; break;
      case rfbDepth24Bit: sz *= 4; break;
      case rfbDepth32Bit: sz *= 4; break;
      case rfbDepthFloat: sz *= 4; break;
   }
   std::vector<char> buf(sz);
   n = rfbReadExact(cl, &buf[0], msg.size);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleDepthMessage: read data");
      rfbCloseClient(cl);
      return TRUE;
   }

   if (msg.sendreply) {
      sendDepthMessage(cl);
   }

   return TRUE;
}
#endif

bool VncServerPlugin::vncAppMessageHandler(int type, const std::vector<char> &msg) {

   switch (type) {
#if 0
      case rfbScreenConfig:
      {
         appScreenConfig app;
         memcpy(&app, &msg[0], sizeof(app));
         coVRConfig::instance()->setNearFar(app.near, app.far);
         osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[0].window;
         int x,y,w,h;
         win->getWindowRectangle(x, y, w, h);
         win->setWindowRectangle(x, y, app.width, app.height);

         opencover::screenStruct &screen = coVRConfig::instance()->screens[0];
         screen.hsize = app.hsize;
         screen.vsize = app.vsize;
         screen.xyz[0] = app.screenPos[0];
         screen.xyz[1] = app.screenPos[1];
         screen.xyz[2] = app.screenPos[2];
         screen.hpr[0] = app.screenRot[0];
         screen.hpr[1] = app.screenRot[1];
         screen.hpr[2] = app.screenRot[2];
      }
      break;
#endif
      case rfbFeedback:
      {
         appFeedback app;
         size_t idx = 0;
         memcpy(&app, &msg[0], sizeof(app));
         idx += sizeof(app);
         std::string info(&msg[idx], app.infolen);
         idx += app.infolen;
         std::string key(&msg[idx], app.keylen);
         idx += app.keylen;
         std::string data(&msg[idx], app.datalen);
         idx += app.datalen;

#ifdef HAVE_COVISE
         CoviseRender::set_feedback_info(info.c_str());
         CoviseRender::send_feedback_message(key.c_str(), data.c_str());
#endif
      }
      break;
   }
   return true;
}

#if 0
//! handle generic application message
rfbBool VncServerPlugin::handleApplicationMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbApplication)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   cd->supportsApplication = true;

   applicationMsg msg;
   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleApplicationMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }
   std::vector<char> buf(msg.size);
   n = rfbReadExact(cl, &buf[0], msg.size);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleApplicationMessage: read data");
      rfbCloseClient(cl);
      return TRUE;
   }

   switch (msg.appType) {
      case rfbScreenConfig:
      {
         appScreenConfig app;
         memcpy(&app, &buf[0], sizeof(app));
         coVRConfig::instance()->setNearFar(app.near, app.far);
         osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[0].window;
         int x,y,w,h;
         win->getWindowRectangle(x, y, w, h);
         win->setWindowRectangle(x, y, app.width, app.height);

         opencover::screenStruct &screen = coVRConfig::instance()->screens[0];
         screen.hsize = app.hsize;
         screen.vsize = app.vsize;
         screen.xyz[0] = app.screenPos[0];
         screen.xyz[1] = app.screenPos[1];
         screen.xyz[2] = app.screenPos[2];
         screen.hpr[0] = app.screenRot[0];
         screen.hpr[1] = app.screenRot[1];
         screen.hpr[2] = app.screenRot[2];
      }
      break;
      case rfbFeedback:
      {
         appFeedback app;
         size_t idx = 0;
         memcpy(&app, &buf[0], sizeof(app));
         idx += sizeof(app);
         std::string info(&buf[idx], app.infolen);
         idx += app.infolen;
         std::string key(&buf[idx], app.keylen);
         idx += app.keylen;
         std::string data(&buf[idx], app.datalen);
         idx += app.datalen;

#ifdef HAVE_COVISE
         CoviseRender::set_feedback_info(info.c_str());
         CoviseRender::send_feedback_message(key.c_str(), data.c_str());
#endif
      }
      break;
   }

   return TRUE;
}
#endif

#if 0
//! send bounding sphere of scene to a client
static void sendBoundsMessage(rfbClientPtr cl) {

   boundsMsg msg;
   msg.type = rfbBounds;
   osg::BoundingSphere bs = VRSceneGraph::instance()->getBoundingSphere();
   msg.center[0] = bs.center()[0];
   msg.center[1] = bs.center()[1];
   msg.center[2] = bs.center()[2];
   msg.radius = bs.radius();
   rfbWriteExact(cl, (char *)&msg, sizeof(msg));
}

//! handle request for a bounding sphere update
rfbBool VncServerPlugin::handleBoundsMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbBounds)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   cd->supportsBounds = true;

   boundsMsg msg;
   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleBoundsMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }

   if (msg.sendreply) {
      sendBoundsMessage(cl);
   }

   return TRUE;
}
#endif


//! handler for VNC key event
void VncServerPlugin::keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl)
{
   static int modifiermask = 0;
   int modifierbit = 0;
   switch(sym) {
      case 0xffe1: // shift
         modifierbit = 0x01;
         break;
      case 0xffe3: // control
         modifierbit = 0x04;
         break;
      case 0xffe7: // meta
         modifierbit = 0x40;
         break;
      case 0xffe9: // alt
         modifierbit = 0x10;
         break;
   }
   if (modifierbit) {
      if (down)
         modifiermask |= modifierbit;
      else
         modifiermask &= ~modifierbit;
   }
   fprintf(stderr, "key %d %s, mod=%02x\n", sym, down?"down":"up", modifiermask);
   OpenCOVER::instance()->handleEvents(down ? osgGA::GUIEventAdapter::KEYDOWN : osgGA::GUIEventAdapter::KEYUP, modifiermask, sym);
}


//! handler for VNC pointer event
void VncServerPlugin::pointerEvent(int buttonmask, int ex, int ey, rfbClientPtr cl)
{
   // necessary to update other clients
   rfbDefaultPtrAddEvent(buttonmask, ex, ey, cl);

   static int lastmask = 0;
   int x,y,w,h;
   osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[0].window;
   win->getWindowRectangle(x,y,w,h);
   Input::instance()->mouse()->handleEvent(osgGA::GUIEventAdapter::DRAG, ex, h-1-ey);
   int changed = lastmask ^ buttonmask;
   lastmask = buttonmask;

   for(int i=0; i<3; ++i) {
      if (changed&1) {
         Input::instance()->mouse()->handleEvent((buttonmask&1) ? osgGA::GUIEventAdapter::PUSH : osgGA::GUIEventAdapter::RELEASE, lastmask, 0);
         fprintf(stderr, "button %d %s\n", i+1, buttonmask&1 ? "push" : "release");
      }
      buttonmask >>= 1;
      changed >>= 1;
   }
}


//! let all connected clients know when a COVISE object was added
void
VncServerPlugin::broadcastAddObject(RenderObject *ro,
      bool isBase,
      RenderObject *geomObj,
      RenderObject *normObj,
      RenderObject *colorObj,
      RenderObject *texObj)
{
   if (!ro)
      return;

   appAddObject app;
   app.isbase = isBase ? 1 : 0;
   app.nattrib = ro->getNumAttributes();
   const char *objName = ro->getName();
   app.namelen = strlen(objName);
   app.geonamelen = 0;
   app.normnamelen = 0;
   app.colnamelen = 0;
   app.texnamelen = 0;

   std::vector<char> buf;
   std::copy((char *)&app, ((char *)&app)+sizeof(app), std::back_inserter(buf));
   std::copy(objName, objName+app.namelen, std::back_inserter(buf));

   if (geomObj) {
      const char *name = geomObj->getName();
      app.geonamelen = strlen(name);
      std::copy(name, name+app.geonamelen, std::back_inserter(buf));
   }
   if (normObj) {
      const char *name = normObj->getName();
      app.normnamelen = strlen(name);
      std::copy(name, name+app.normnamelen, std::back_inserter(buf));
   }
   if (colorObj) {
      const char *name = colorObj->getName();
      app.colnamelen = strlen(name);
      std::copy(name, name+app.colnamelen, std::back_inserter(buf));
   }
   if (texObj) {
      const char *name = texObj->getName();
      app.texnamelen = strlen(name);
      std::copy(name, name+app.texnamelen, std::back_inserter(buf));
   }
   memcpy(&buf[0], &app, sizeof(app));

   for (size_t i=0; i<ro->getNumAttributes(); ++i) {
      const char *name = ro->getAttributeName(i);
      const char *value = ro->getAttributeValue(i);
      appAttribute attr;
      attr.namelen = strlen(name);
      attr.valuelen = strlen(value);
      std::copy((char *)&attr, ((char *)&attr)+sizeof(attr), std::back_inserter(buf));
      std::copy(name, name+attr.namelen, std::back_inserter(buf));
      std::copy(value, value+attr.valuelen, std::back_inserter(buf));
   }

   plugin->m_vnc->broadcastApplicationMessage(rfbAddObject, buf.size(), &buf[0]);
}

//! this function is called when ever a COVISE Object is received
void
VncServerPlugin::addObject(RenderObject *baseObj,
      RenderObject *geomObj, RenderObject *normObj,
      RenderObject *colorObj, RenderObject *texObj,
      osg::Group *parent,
      int numCol, int colorBinding, int colorPacking,
      float *r, float *g, float *b, int *packedCol,
      int numNormals, int normalBinding,
      float *xn, float *yn, float *zn,
      float transparency)
{
   //std::cerr << "VncServer: addObject(" << baseObj->getName();
   //std::cerr << ")" << std::endl;

   broadcastAddObject(geomObj);
   broadcastAddObject(normObj);
   broadcastAddObject(colorObj);
   broadcastAddObject(texObj);

   broadcastAddObject(baseObj, true, geomObj, normObj, colorObj, texObj);
}

//! this function is called if a COVISE Object is removed
void
VncServerPlugin::removeObject(const char *objName, bool replaceFlag)
{
   //std::cerr << "VncServer: removeObject(" << objName;
   //std::cerr << ")" << std::endl;

   appRemoveObject app;
   app.namelen = strlen(objName);
   app.replace = replaceFlag ? 1 : 0;
   std::vector<char> buf;
   std::back_insert_iterator<std::vector<char> > back_inserter(buf);
   std::copy((char *)&app, ((char *)&app)+sizeof(app), back_inserter);
   std::copy(objName, objName+app.namelen, back_inserter);

   m_vnc->broadcastApplicationMessage(rfbRemoveObject, buf.size(), &buf[0]);
}

//! this is called before every frame, used for polling for RFB messages
void
VncServerPlugin::preFrame()
{
   if (m_delay) {
      usleep(m_delay);
   }

   m_vnc->preFrame();

#if 0
   const int wait_msec=0;

   rfbCheckFds(m_screen, wait_msec*1000);
   rfbHttpCheckFds(m_screen);

   rfbClientIteratorPtr i = rfbGetClientIterator(m_screen);
   while (rfbClientPtr cl = rfbClientIteratorNext(i)) {
      if (rfbUpdateClient(cl)) {
         struct ClientData *cd = static_cast<ClientData *>(cl->clientData);
         //std::cerr << "it: " << c++ << ", depth: " << (cd && cd->supportsDepth) << std::endl;
         if (cd && cd->supportsDepth)
            sendDepthMessage(cl);
      }
   }
   rfbReleaseClientIterator(i);
#endif

#if 0
   //fprintf(stderr, "VncServer preFrame\n");
   int nevents = 0;
   while (rfbProcessEvents(m_screen, 0))
      ++nevents;

   if (nevents > 0) {
      rfbClientIteratorPtr it = rfbGetClientIterator(m_screen);
      int numclients=0;
      while (rfbClientPtr cl = rfbClientIteratorNext(it)) {
         ++numclients;
         struct ClientData *cd = static_cast<ClientData *>(cl->clientData);
         //std::cerr << "it: " << c++ << ", depth: " << (cd && cd->supportsDepth) << std::endl;
         if (cd && cd->supportsDepth)
            sendDepthMessage(cl);
      }
      rfbReleaseClientIterator(it);
   }
#endif

#if 0
   while (rfbProcessEvents(m_screen, 0))
      ;
#endif

#if 0
   if (numclients == 0) {
      for (int i=0; i<m_clientList.size(); ++i) {
         if (rfbReverseConnection(m_screen, const_cast<char *>(m_clientList[i].host.c_str()), m_clientList[i].port)) {
            break;
         }
      }
   }
#endif
}

//! mirror image vertically
template<typename T>
static void flip_upside_down(T *buf, unsigned w, unsigned h, unsigned bpp)
{
   T *front = buf;
   T *back = &buf[w * (h-1) * bpp];
   std::vector<T> temp(w*bpp);

   while (front < back) {
      memcpy(&temp[0], front, w*bpp*sizeof(T));
      memcpy(front, back, w*bpp*sizeof(T));
      memcpy(back, &temp[0], w*bpp*sizeof(T));

      front += w*bpp;
      back -= w*bpp;
   }
}

//! called after back-buffer has been swapped to front-buffer
void
VncServerPlugin::postSwapBuffers(int windowNumber)
{
   if(windowNumber != 0)
      return;
   if(!coVRMSController::instance()->isMaster())
      return;

   if (m_depthcopy && !m_glextChecked) {
      m_glextChecked = true;
      unsigned int contextID = coVRConfig::instance()->windows[windowNumber].window->getState()->getContextID();
      if (!osg::isGLExtensionSupported(contextID, "GL_NV_copy_depth_to_color")) {
         m_depthcopy = false;
      } else {
         m_depthprecision = 32;
         m_depthfloat = false;
      }
      std::cerr << "GL_NV_copy_depth_to_color " << (m_depthcopy ? "enabled" : "disabled") << std::endl;
   }

   osgViewer::GraphicsWindow *win = coVRConfig::instance()->windows[windowNumber].window;
   int x,y,w,h;
   win->getWindowRectangle(x, y, w, h);
#if 0
   if (!m_screen->frameBuffer || m_width != w || m_height != h) {
      char *oldfb = m_screen->frameBuffer;
      bool deletecuda = false;
      if (m_cudapinnedmemory) {
#ifdef HAVE_CUDA
         deletecuda = true;
         cudaError_t err = cudaHostAlloc((void**)&m_screen->frameBuffer, w*h*4*2, cudaHostAllocPortable);
         if (err != cudaSuccess) {
            std::cerr << "VncServer: allocation of " << w*h*4*2 << " bytes of CUDA pinned memory failed: "
               << cudaGetErrorString(err) << std::endl;
            m_cudapinnedmemory = false;
         }
#endif
      }
      if (!m_cudapinnedmemory) {
         m_screen->frameBuffer = new char[w*h*4*2]; // RGBA + (Z+stencil or Z as float)
      }

      rfbNewFramebuffer(m_screen, m_screen->frameBuffer,
            w, h, 8, 3, 4);
      m_width = w;
      m_height = h;

      if (deletecuda) {
#ifdef HAVE_CUDA
         cudaFreeHost(oldfb);
#endif
      } else {
         delete[] oldfb;
      }
   }
#endif

   double start = 0.;
   if(m_benchmark)
      start = cover->currentTime();
   double bpp = 4.;
   GLint buf = GL_FRONT;
   GLenum format = GL_RGBA;
   GLenum depthtype=GL_FLOAT;
   GLenum depthformat=GL_DEPTH_COMPONENT;
   int depthps = 4;
   if (!m_depthfloat) {
      switch (m_depthprecision) {
         case 8: depthtype=GL_UNSIGNED_BYTE; depthps=1; break;
         case 16: depthtype=GL_UNSIGNED_SHORT; depthps=2; break;
         case 24: depthtype=GL_UNSIGNED_INT_24_8; depthps=4;
                  depthformat=GL_DEPTH_STENCIL;
                  break;
         case 32: depthtype=GL_UNSIGNED_INT; depthps=4; break;
      }
   }

   if(m_cudaColor) {
      if (!m_cudaColor->isInitialized()) {
         if (!m_cudaColor->init()) {
            delete m_cudaColor;
            m_cudaColor = NULL;
         }
      }
   }
   if(m_cudaDepth) {
      if (!m_cudaDepth->isInitialized()) {
         if (!m_cudaDepth->init()) {
            delete m_cudaDepth;
            m_cudaDepth = NULL;
         }
      }
   }

   if(m_cudaColor) {
      m_cudaColor->readpixels(0, 0, m_width, m_width, m_height, format,
            4, m_vnc->rgba(0), buf);
   } else {
      readpixels(0, 0, m_width, m_width, m_height, format,
            4, m_vnc->rgba(0), buf);
   }
   flip_upside_down(m_vnc->rgba(0), m_width, m_height, 4);

   double colorfinish = 0.;
   if (m_benchmark)
      colorfinish = cover->currentTime();

   if (m_vnc->numRhrClients() > 0) {
      if (m_depthcopy) {
         glCopyPixels(0, 0, m_width, m_height, GL_DEPTH_STENCIL_TO_RGBA_NV);
         depthformat = GL_BGRA;
         depthtype = GL_UNSIGNED_BYTE;
         depthps = 4;
      }
      if(m_cudaDepth) {
         if (m_depthquant) {
            m_cudaDepth->readdepthquant(0, 0, m_width, m_width, m_height, depthformat,
                  depthps, (GLubyte *)m_vnc->depth(0), buf, depthtype);
         } else {
            m_cudaDepth->readpixels(0, 0, m_width, m_width, m_height, depthformat,
                  depthps, (GLubyte *)m_vnc->depth(0), buf, depthtype);
            flip_upside_down((GLubyte *)m_vnc->depth(0), m_width, m_height, depthps);
         }
      } else {
         readpixels(0, 0, m_width, m_width, m_height, depthformat,
               depthps, (GLubyte *)m_vnc->depth(0), buf, depthtype);
         flip_upside_down((GLubyte *)m_vnc->depth(0), m_width, m_height, depthps);
      }
   }

   m_vnc->invalidate(0, 0, 0, m_vnc->width(0), m_vnc->height(0), m_vncParam, true);

   double pix = m_width*m_height;
   double bytes = pix * bpp;
   if(m_benchmark || m_errormetric || m_compressionrate) {
      double colordur = colorfinish - start;
      if (m_benchmark) {
         fprintf(stderr, "COLOR %fs: %f mpix/s, %f gb/s (cuda=%d)\n",
               colordur,
               pix/colordur/1e6, bytes/colordur/(1024*1024*1024),
               m_cudaColor!=NULL);
      }

      if (m_vnc->numRhrClients() > 0) {
         double depthdur = cover->currentTime() - colorfinish;

         const size_t sz = m_width*m_height*depthps;
         std::vector<char> depth(sz);
         readpixels(0, 0, m_width, m_width, m_height, depthformat,
               depthps, (GLubyte *)&depth[0], buf, depthtype);
         flip_upside_down(&depth[0], m_width, m_height, depthps);

#ifdef HAVE_SNAPPY
         size_t maxsz = snappy::MaxCompressedLength(sz);
         std::vector<char> compressed(maxsz);

         double snappystart = cover->currentTime();
         size_t compressedsz = 0;
         snappy::RawCompress(&depth[0], sz, &compressed[0], &compressedsz);
         double snappydur = cover->currentTime() - snappystart;
         if (m_compressionrate) {
            fprintf(stderr, "snappy solo: %ld -> %ld, %f s, %f gb/s\n", sz, compressedsz, snappydur,
                  sz/snappydur/1024/1024/1024);
         }
#endif

#if 0
         if (m_errormetric && m_depthquant) {
            std::vector<char> dequant(m_width*m_height*depthps);
            depthdequant(&dequant[0], m_vnc->depth(0), depthps, 0, 0, m_width, m_height);
            depthcompare(&depth[0], &dequant[0], m_depthprecision, m_width, m_height);
         }
#endif

         double bytesraw = pix * depthps;
         double bytesread = pix;
         int structsz = 1;
         if (m_cudaDepth && m_depthquant) {
            int edge = 1;
            if (depthps <= 2) {
               edge = DepthQuantize16::edge;
               structsz = sizeof(DepthQuantize16);
            } else {
               edge = DepthQuantize24::edge;
               structsz = sizeof(DepthQuantize24);
            }
            bytesread *= structsz;
            bytesread /= edge*edge;
         } else {
            bytesread *= depthps;
         }

         if (m_benchmark) {
            fprintf(stderr, "DEPTH %fs: %f mpix/s, %f gb/s raw, %f gb/s read (cuda=%d) (ps=%d, ts=%d)\n",
                  depthdur,
                  pix/depthdur/1e6,
                  bytesraw/depthdur/(1024*1024*1024),
                  bytesread/depthdur/(1024*1024*1024),
                  m_cudaDepth!=NULL, depthps, structsz);
         }
      }
   }
}

void VncServerPlugin::key(int type, int keySym, int mod) {

   if (type == osgGA::GUIEventAdapter::KEYDOWN) {
      if ((mod & osgGA::GUIEventAdapter::MODKEY_SHIFT)
            && (mod | osgGA::GUIEventAdapter::MODKEY_SHIFT) == osgGA::GUIEventAdapter::MODKEY_SHIFT) {
         if (keySym == 'D') {
            m_depthquant = !m_depthquant;
            std::cerr << "toggled depth quantization: " << (m_depthquant?"on":"off") << std::endl;
         }
         if (keySym == 'W') {
            m_delay += 500000;
            if (m_delay > 1000000) {
               m_delay = 0;
            }
            std::cerr << "set delay to " << m_delay << " us" << std::endl;
         }
      }
   }
}

//! OpenGL framebuffer read-back
void VncServerPlugin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
      GLenum format, int ps, GLubyte *bits, GLint buf, GLenum type)
{

   GLint readbuf=GL_BACK;
   glGetIntegerv(GL_READ_BUFFER, &readbuf);

   glReadBuffer(buf);
   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

   if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
   else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
   else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
   else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

   // Clear previous error
   while (glGetError() != GL_NO_ERROR)
      ;

   glReadPixels(x, y, w, h, format, type, bits);

   glPopClientAttrib();
   glReadBuffer(readbuf);
}

COVERPLUGIN(VncServerPlugin)
