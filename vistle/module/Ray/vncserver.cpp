/**\file
 * \brief VncServer plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014 HLRS
 *
 * \copyright GPL2+
 */

#include <iostream>
#include <core/assert.h>
#include <cmath>

#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

#include <RHR/rfbext.h>
#include <RHR/depthquant.h>

#include "vncserver.h"

#include <boost/chrono.hpp>

static double now() {
    namespace chrono = boost::chrono;
    typedef chrono::high_resolution_clock clock_type;
    const clock_type::time_point now = clock_type::now();
    return 1e-9*chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch()).count();
}

VncServer *VncServer::plugin = NULL;

//! per-client data: supported extensions
struct ClientData {
   ClientData()
      : supportsBounds(false)
      , supportsDepth(false)
      , depthCompressions(0)
      , supportsApplication(false)
      , supportsLights(false)
   {
   }
   bool supportsBounds;
   bool supportsDepth;
   uint8_t depthCompressions;
   std::vector<char> buf;
   bool supportsApplication;
   bool supportsLights;
};

static rfbProtocolExtension matricesExt = {
   NULL, // newClient
   NULL, // init
   matricesEncodings, // pseudoEncodings
   VncServer::enableMatrices, // enablePseudoEncoding
   VncServer::handleMatricesMessage, // handleMessage
   NULL, // close
   NULL, // usage
   NULL, // processArgument
   NULL, // next extension
};

static rfbProtocolExtension lightsExt = {
   NULL, // newClient
   NULL, // init
   lightsEncodings, // pseudoEncodings
   VncServer::enableLights, // enablePseudoEncoding
   VncServer::handleLightsMessage, // handleMessage
   NULL, // close
   NULL, // usage
   NULL, // processArgument
   NULL, // next extension
};

static rfbProtocolExtension boundsExt = {
   NULL, // newClient
   NULL, // init
   boundsEncodings, // pseudoEncodings
   VncServer::enableBounds, // enablePseudoEncoding
   VncServer::handleBoundsMessage, // handleMessage
   NULL, // close
   NULL, // usage
   NULL, // processArgument
   NULL, // next extension
};

static rfbProtocolExtension depthExt = {
   NULL, // newClient
   NULL, // init
   depthEncodings, // pseudoEncodings
   VncServer::enableDepth, // enablePseudoEncoding
   VncServer::handleDepthMessage, // handleMessage
   NULL, // close
   NULL, // usage
   NULL, // processArgument
   NULL, // next extension
};

static rfbProtocolExtension applicationExt = {
   NULL, // newClient
   NULL, // init
   applicationEncodings, // pseudoEncodings
   VncServer::enableApplication, // enablePseudoEncoding
   VncServer::handleApplicationMessage, // handleMessage
   NULL, // close
   NULL, // usage
   NULL, // processArgument
   NULL, // next extension
};


//! called when plugin is loaded
VncServer::VncServer(int w, int h, unsigned short port)
{
   vassert(plugin == NULL);
   plugin = this;

   //fprintf(stderr, "new VncServer plugin\n");

   init(w, h, port);
}

// this is called if the plugin is removed at runtime
VncServer::~VncServer()
{
   vassert(plugin);

   rfbShutdownServer(m_screen, true);

   delete[] m_screen->frameBuffer;

   //fprintf(stderr,"VncServer::~VncServer\n");

   plugin = nullptr;
}

void VncServer::enableQuantization(bool value) {

    m_depthquant = value;
}

void VncServer::enableSnappy(bool value) {

    m_depthsnappy = value;
}

void VncServer::setDepthPrecision(int bits) {

    m_depthprecision = bits;
}

unsigned short VncServer::port() const {

   return m_screen->port;
}


unsigned char *VncServer::rgba() const {

   vassert(m_screen->frameBuffer);
   return (unsigned char *)m_screen->frameBuffer;
}

float *VncServer::depth() {

   return m_depth.data();
}

const float *VncServer::depth() const {

   return m_depth.data();
}

int VncServer::width() const {

   return m_width;
}

int VncServer::height() const {

   return m_height;
}

const vistle::Matrix4 &VncServer::viewMat() const {

   return m_viewMat;
}

const vistle::Matrix4 &VncServer::projMat() const {

   return m_projMat;
}

const vistle::Matrix4 &VncServer::scaleMat() const {

   return m_scaleMat;
}

const vistle::Matrix4 &VncServer::transformMat() const {

   return m_transformMat;
}

const vistle::Matrix4 &VncServer::viewerMat() const {

   return m_viewerMat;
}

void VncServer::setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius) {

   m_boundCenter = center;
   m_boundRadius = radius;
}

const VncServer::Screen &VncServer::screen() const {

   return m_screenConfig;
}

//! called after plug-in is loaded and scenegraph is initialized
bool VncServer::init(int w, int h, unsigned short port) {

   m_numRhrClients = 0;
   m_boundCenter = vistle::Vector3(0., 0., 0.);
   m_boundRadius = 1.;
   m_depthSent = false;

   m_viewMat = vistle::Matrix4::Identity();
   m_projMat = vistle::Matrix4::Identity();
   m_transformMat = vistle::Matrix4::Identity();
   m_scaleMat = vistle::Matrix4::Identity();
   m_viewerMat = vistle::Matrix4::Identity();

   double zNear = 1.;
   m_projMat <<  zNear,0,0,0,
   0,zNear,0,0,
   0,0,-11./9.,-20./9.,
   0,0,-1,0;

   rfbRegisterProtocolExtension(&matricesExt);
   rfbRegisterProtocolExtension(&lightsExt);
   rfbRegisterProtocolExtension(&boundsExt);
   rfbRegisterProtocolExtension(&depthExt);
   rfbRegisterProtocolExtension(&applicationExt);

   m_delay = 0;
   m_lastMatrixTime = 0.;
   std::string config("COVER.Plugin.VncServer");

   m_benchmark = false;
   m_errormetric = false;
   m_compressionrate = false;

   m_depthprecision = 32;
   m_depthfloat = true;
   m_depthquant = true;
   m_depthsnappy = true;

   m_width = 0;
   m_height = 0;

   int argc = 1;
   char *argv[] = { (char *)"DisCOVERay", NULL };

   m_screen = rfbGetScreen(&argc, argv, 0, 0, 8, 3, 4);
   m_screen->desktopName = "DisCOVERay";

   m_screen->autoPort = FALSE;
   m_screen->port = port;
   m_screen->ipv6port = port;

   m_screen->kbdAddEvent = &keyEvent;
   m_screen->ptrAddEvent = &pointerEvent;

   m_screen->deferUpdateTime = 0;
   m_screen->maxRectsPerUpdate = 10000000;
   rfbInitServer(m_screen);
   m_screen->deferUpdateTime = 0;
   m_screen->maxRectsPerUpdate = 10000000;
   m_screen->handleEventsEagerly = 1;

   m_screen->cursor = NULL; // don't show a cursor

   std::string host;
   if(!host.empty())
   {
      Client c;
      c.host = host;
      c.port = 31042;
      m_clientList.push_back(c);
   }
   //host = covise::coCoviseConfig::getEntry("alternateClientHost", config);
   if(!host.empty())
   {
      Client c;
      c.host = host;
      c.port = 31042;
      m_clientList.push_back(c);
   }

   resize(w, h);

   return true;
}


void VncServer::resize(int w, int h) {

   if (m_width != w || m_height != h) {
      m_width = w;
      m_height = h;

      delete[] m_screen->frameBuffer;
      m_screen->frameBuffer = new char[w*h*4];

      m_depth.resize(w*h);

      rfbNewFramebuffer(m_screen, m_screen->frameBuffer,
                     w, h, 8, 3, 4);

   }
}


//! clean up per-client data when client disconnects
void VncServer::clientGoneHook(rfbClientPtr cl) {
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (cd->supportsDepth) {
      std::cerr << "VncServer: RHR client gone" << std::endl;
      --plugin->m_numRhrClients;
   }
   delete cd;
}

//! enable sending of matrices
rfbBool VncServer::enableMatrices(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbMatrices)
      return FALSE;

   matricesMsg msg;
   msg.type = rfbMatrices;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableMatrices: write");
   }

   return TRUE;
}

//! handle matrix update message
rfbBool VncServer::handleMatricesMessage(rfbClientPtr cl, void *data,
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

   for (int i=0; i<16; ++i) {
      plugin->m_scaleMat.data()[i] = msg.scale[i];
      plugin->m_transformMat.data()[i] = msg.transform[i];
      plugin->m_viewerMat.data()[i] = msg.viewer[i];
      plugin->m_viewMat.data()[i] = msg.view[i];
      plugin->m_projMat.data()[i] = msg.proj[i];
   }

   return TRUE;
}

//! enable sending of lighting parameters
rfbBool VncServer::enableLights(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbLights)
      return FALSE;

   lightsMsg msg;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableLights: write");
   }

   return TRUE;
}

//! handle light update message
rfbBool VncServer::handleLightsMessage(rfbClientPtr cl, void *data,
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

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   cd->supportsLights = true;

#define SET_VEC(d, dst, src) \
      do { \
         for (int k=0; k<d; ++k) { \
            (dst)[k] = (src)[k]; \
         } \
      } while(false)

   plugin->lights.clear();
   for (int i=0; i<lightsMsg::NumLights; ++i) {

      const auto &cl = msg.lights[i];
      if (cl.enabled) {
         plugin->lights.push_back(Light());
         auto &l = plugin->lights.back();

         SET_VEC(4, l.position, cl.position);
         SET_VEC(4, l.ambient, cl.ambient);
         SET_VEC(4, l.diffuse, cl.diffuse);
         SET_VEC(4, l.specular, cl.specular);
         SET_VEC(3, l.attenuation, cl.attenuation);
         SET_VEC(3, l.direction, cl.spot_direction);
         l.spotCutoff = cl.spot_cutoff;
         l.spotExponent = cl.spot_exponent;

         //std::cerr << "Light " << i << ": ambient: " << l.ambient << std::endl;
         //std::cerr << "Light " << i << ": diffuse: " << l.diffuse << std::endl;
      }
   }

   //std::cerr << "handleLightsMessage: " << plugin->lights.size() << " lights received" << std::endl;

   return TRUE;
}

//! enable sending of depth buffer
rfbBool VncServer::enableDepth(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbDepth)
      return FALSE;

   depthMsg msg;
   msg.type = rfbDepth;
   msg.x = 0;
   msg.y = 0;
   msg.width = 0;
   msg.height = 0;
   msg.size = 0;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableDepth: write");
   }

   return TRUE;
}

//! send depth buffer to a client
void VncServer::sendDepthMessage(rfbClientPtr cl) {

   if (!plugin->m_screen->frameBuffer)
      return;

   depthMsg msg;
   msg.type = rfbDepth;
   msg.requesttime = plugin->m_lastMatrixTime;
#if 0
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
   msg.size = sz;

   const char *zbuf = (const char *)plugin->depth();
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (plugin->m_depthquant && (cd->depthCompressions&rfbDepthQuantize)) {
      msg.compression |= rfbDepthQuantize;
      const int ds = plugin->m_depthprecision<=16 ? 2 : 3;
      msg.format = ds==2 ? rfbDepth16Bit : rfbDepth24Bit;
      msg.size = depthquant_size(DepthFloat, ds, msg.width, msg.height);
      std::cerr << "required size for quant: " << msg.size << std::endl;
      if (msg.size > plugin->m_quant.size())
          plugin->m_quant.resize(msg.size);
      depthquant(plugin->m_quant.data(), zbuf, DepthFloat, ds, 0, 0, msg.width, msg.height);
      zbuf = plugin->m_quant.data();
      sz = msg.size;
   }
   double snappyduration = 0.;
#ifdef HAVE_SNAPPY
   if(plugin->m_depthsnappy && (cd->depthCompressions & rfbDepthSnappy)) {
      if (snappy::MaxCompressedLength(msg.size) > cd->buf.size()) {
         cd->buf.resize(snappy::MaxCompressedLength(msg.size));
      }
      double snappystart = 0.;
      if (plugin->m_benchmark)
         snappystart = now();
      size_t compressed = 0;
      snappy::RawCompress(zbuf, msg.size, cd->buf.data(), &compressed);
      if (plugin->m_benchmark)
         snappyduration = now() - snappystart;
      //std::cerr << "compressed " << msg.size << " to " << compressed << " (buf: " << cd->buf.size() << ")" << std::endl;
      msg.compression |= rfbDepthSnappy;
      msg.size = compressed;
      zbuf = cd->buf.data();
   }
#endif
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
       rfbLogPerror("sendDepthMessage: write header");
   }
   if (rfbWriteExact(cl, zbuf, msg.size) < 0) {
       rfbLogPerror("sendDepthMessage: write paylod");
   }

   if (plugin->m_benchmark || plugin->m_compressionrate) {
      fprintf(stderr, "Depth: raw=%ld, quant=%ld, snappy=%ld, %f s, %f gb/s\n",
            (long)msg.width*msg.height*plugin->m_depthprecision/8,
            (long)sz,
            (long)msg.size,
            snappyduration,
            snappyduration > 0. ? sz/snappyduration/1024/1024/1024 : 0.);
   }

   plugin->m_depthSent = true;
}

//! handle depth request by client
rfbBool VncServer::handleDepthMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbDepth)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (!cd->supportsDepth) {
      std::cerr << "VncServer: RHR client connected" << std::endl;
      ++plugin->m_numRhrClients;
   }
   cd->supportsDepth = true;

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

//! enable generic application messages
rfbBool VncServer::enableApplication(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbApplication)
      return FALSE;

   applicationMsg msg;
   msg.type = rfbApplication;
   msg.appType = 0;
   msg.sendreply = 0;
   msg.version = 0;
   msg.size = 0;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableApplication: write");
   }

   return TRUE;
}

//! send generic application message to a client
void VncServer::sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data) {

   applicationMsg msg;
   msg.type = rfbApplication;
   msg.appType = type;
   msg.sendreply = 0;
   msg.version = 0;
   msg.size = length;

   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
      rfbLogPerror("sendApplicationMessage: write");
   }
   if (rfbWriteExact(cl, data, msg.size) < 0) {
      rfbLogPerror("sendApplicationMessage: write data");
   }
}

//! send generic application message to all connected clients
void VncServer::broadcastApplicationMessage(int type, int length, const char *data) {

   rfbClientIteratorPtr it = rfbGetClientIterator(m_screen);
   while (rfbClientPtr cl = rfbClientIteratorNext(it)) {
      struct ClientData *cd = static_cast<ClientData *>(cl->clientData);
      if (cd && cd->supportsApplication)
         sendApplicationMessage(cl, type, length, data);
   }
   rfbReleaseClientIterator(it);
}

//! handle generic application message
rfbBool VncServer::handleApplicationMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbApplication)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
      cl->clientGoneHook = clientGoneHook;

      appAnimationTimestep app;
      app.current = plugin->m_timestep;
      app.total = plugin->m_numTimesteps;
      sendApplicationMessage(cl, rfbAnimationTimestep, sizeof(app), (char *)&app);
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
         plugin->resize(app.width, app.height);
         plugin->m_screenConfig.hsize = app.hsize;
         plugin->m_screenConfig.vsize = app.vsize;
         for (int i=0; i<3; ++i) {
            plugin->m_screenConfig.pos[i] = app.screenPos[i];
            plugin->m_screenConfig.hpr[i] = app.screenRot[i];
         }
      }
      break;
      case rfbFeedback:
      {
         // not needed: handled via vistle connection
      }
      break;
      case rfbAnimationTimestep: {
         appAnimationTimestep app;
         memcpy(&app, &buf[0], sizeof(app));
         plugin->m_timestep = app.current;
      }
      break;
   }

   return TRUE;
}

int VncServer::timestep() const {

   return m_timestep;
}

void VncServer::setNumTimesteps(int num) {

   if (num != m_numTimesteps) {
      m_numTimesteps = num;
      appAnimationTimestep app;
      app.current = m_timestep;
      app.total = num;
      broadcastApplicationMessage(rfbAnimationTimestep, sizeof(app), (char *)&app);
   }
}

//! send bounding sphere of scene to a client
void VncServer::sendBoundsMessage(rfbClientPtr cl) {

#if 0
   std::cerr << "sending bounds: "
             << "c: " << plugin->m_boundCenter
             << "r: " << plugin->m_boundRadius << std::endl;
#endif

   boundsMsg msg;
   msg.type = rfbBounds;
   msg.center[0] = plugin->m_boundCenter[0];
   msg.center[1] = plugin->m_boundCenter[1];
   msg.center[2] = plugin->m_boundCenter[2];
   msg.radius = plugin->m_boundRadius;
   rfbWriteExact(cl, (char *)&msg, sizeof(msg));
}


//! enable bounding sphere messages
rfbBool VncServer::enableBounds(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbBounds)
      return FALSE;

   boundsMsg msg;
   msg.type = rfbBounds;
   msg.center[0] = 0.;
   msg.center[1] = 0.;
   msg.center[2] = 0.;
   msg.radius = 0.;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableBounds: write");
   }

   return TRUE;
}

//! handle request for a bounding sphere update
rfbBool VncServer::handleBoundsMessage(rfbClientPtr cl, void *data,
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


//! handler for VNC key event
void VncServer::keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl)
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
   //OpenCOVER::instance()->handleEvents(down ? osgGA::GUIEventAdapter::KEYDOWN : osgGA::GUIEventAdapter::KEYUP, modifiermask, sym);
}


//! handler for VNC pointer event
void VncServer::pointerEvent(int buttonmask, int ex, int ey, rfbClientPtr cl)
{
   // necessary to update other clients
   rfbDefaultPtrAddEvent(buttonmask, ex, ey, cl);
}


//! this is called before every frame, used for polling for RFB messages
void
VncServer::preFrame()
{
   const int wait_msec=1;

   if (m_delay) {
      usleep(m_delay);
   }

   rfbCheckFds(m_screen, wait_msec*1000);
   rfbHttpCheckFds(m_screen);

   rfbClientIteratorPtr i = rfbGetClientIterator(m_screen);
   while (rfbClientPtr cl = rfbClientIteratorNext(i)) {
      if (rfbUpdateClient(cl)) {
         struct ClientData *cd = static_cast<ClientData *>(cl->clientData);
         //std::cerr << "it: " << c++ << ", depth: " << (cd && cd->supportsDepth) << std::endl;
         if (!m_depthSent && cd && cd->supportsDepth) {
            sendDepthMessage(cl);
         }
      }
   }
   rfbReleaseClientIterator(i);
}

template<typename T>
int clz (T xx)
{
   vassert(sizeof(T)<=8);
   uint64_t x = xx;

   if (xx==0)
      return 8*sizeof(T);

   int n=0;
   if ((x & 0xffffffff00000000UL) == 0) { x <<= 32; if (sizeof(T) >= 8) n +=32; }
   if ((x & 0xffff000000000000UL) == 0) { x <<= 16; if (sizeof(T) >= 4) n +=16; }
   if ((x & 0xff00000000000000UL) == 0) { x <<= 8;  if (sizeof(T) >= 2) n += 8; }
   if ((x & 0xf000000000000000UL) == 0) { x <<= 4;  n += 4; }
   if ((x & 0xc000000000000000UL) == 0) { x <<= 2;  n += 2; }
   if ((x & 0x8000000000000000UL) == 0) { x <<= 1;  n += 1; }
   return n;
}

template<typename T, class Quant>
void depthcompare_t(const T *ref, const T *check, unsigned w, unsigned h) {

#ifdef max
#undef max
#endif

   size_t numlow=0, numhigh=0;
   unsigned maxx=0, maxy=0;
   size_t numblack=0, numblackwrong=0;
   size_t totalvalidbits = 0;
   int minvalidbits=8*sizeof(T), maxvalidbits=0;
   double totalerror = 0.;
   double squarederror = 0.;
   T Max = std::numeric_limits<T>::max();
   if (sizeof(T)==4)
      Max >>= 8;
   T maxerr = 0;
   T refminval = Max, minval = Max;
   T refmaxval = 0, maxval = 0;
   for (unsigned y=0; y<h; ++y) {
      for (unsigned x=0; x<w; ++x) {
         size_t idx = y*w+x;
         T e = 0;

         T r = ref[idx];
         T c = check[idx];
         if (sizeof(T)==4) {
            r >>= 8;
            c >>= 8;
         }
         if (r == Max) {
            ++numblack;
            if (c != r) {
               ++numblackwrong;
               fprintf(stderr, "!B: %u %u (%lu)\n", x, y, (unsigned long)c);
            }
         } else {
            int validbits = clz<T>(r^c);
            if (validbits < minvalidbits)
               minvalidbits = validbits;
            if (validbits > maxvalidbits)
               maxvalidbits = validbits;
            totalvalidbits += validbits;

            if (r < refminval)
               refminval = r;
            if (r > refmaxval)
               refmaxval = r;
            if (c < minval)
               minval = c;
            if (c > maxval)
               maxval = c;
         }
         if (r > c) {
            ++numlow;
            e = r - c;
         } else if (r < c) {
            ++numhigh;
            e = c - r;
         }
         if (e > maxerr) {
            maxx = x;
            maxy = y;
            maxerr = e;
         }
         double err = e;
         totalerror += err;
         squarederror += err*err;
      }
   }

   size_t nonblack = w*h-numblack;
   double MSE = squarederror/w/h;
   double MSE_nonblack = squarederror/nonblack;
   double PSNR = -1., PSNR_nonblack=-1.;
   if (MSE > 0.) {
      PSNR = 10. * (2.*log10(Max) - log10(MSE));
      PSNR_nonblack = 10. * (2.*log10(Max) - log10(MSE_nonblack));
   }

   fprintf(stderr, "ERROR: #high=%ld, #low=%ld, max=%lu (%u %u), total=%f, mean non-black=%f (%f %%)\n",
         (long)numhigh, (long)numlow, (unsigned long)maxerr, maxx, maxy, totalerror,
         totalerror/nonblack,
         totalerror/nonblack*100./(double)Max
         );
   fprintf(stderr, "BITS: totalvalid=%lu, min=%d, max=%d, mean=%f\n",
         totalvalidbits, minvalidbits, maxvalidbits, (double)totalvalidbits/(double)nonblack);

   fprintf(stderr, "PSNR (2x%d+16x%d): %f dB (non-black: %f)\n",
         Quant::precision, Quant::bits_per_pixel,
         PSNR, PSNR_nonblack);

   fprintf(stderr, "RANGE: ref %lu - %lu, act %lu - %lu\n",
         (unsigned long)refminval, (unsigned long)refmaxval,
         (unsigned long)minval, (unsigned long)maxval);
}

static void depthcompare(const char *ref, const char *check, int precision, unsigned w, unsigned h) {

   if (precision == 16) {
      depthcompare_t<uint16_t, DepthQuantize16>((const uint16_t *)ref, (const uint16_t *)check, w, h);
   } else if (precision == 32) {
      depthcompare_t<uint32_t, DepthQuantize24>((const uint32_t *)ref, (const uint32_t *)check, w, h);
   }
}

//! called after back-buffer has been swapped to front-buffer
void
VncServer::postFrame()
{
   double bpp = 4.;
   int depthps = 4;

   double pix = m_width*m_height;
   if(m_benchmark || m_errormetric || m_compressionrate) {
      if (m_numRhrClients > 0) {

#ifdef HAVE_SNAPPY
         const size_t sz = m_width*m_height*depthps;
         size_t maxsz = snappy::MaxCompressedLength(sz);
         std::vector<char> compressed(maxsz);

         double snappystart = now();
         size_t compressedsz = 0;
         snappy::RawCompress((const char *)depth(), sz, &compressed[0], &compressedsz);
         double snappydur = now() - snappystart;
         if (m_compressionrate) {
            fprintf(stderr, "snappy solo: %ld -> %ld, %f s, %f gb/s\n", sz, compressedsz, snappydur,
                  sz/snappydur/1024/1024/1024);
         }
#endif

#if 0
         if (m_errormetric && m_depthquant) {
            std::vector<char> dequant(m_width*m_height*depthps);
            depthdequant(&dequant[0], m_screen->frameBuffer+4*m_width*m_height, depthps, m_width, m_height);
            depthcompare(depth(), &dequant[0], m_depthprecision, m_width, m_height);
         }
#endif

         double bytesraw = pix * depthps;
         double bytesread = pix;
         int structsz = 1;
         bytesread *= depthps;

#if 0
         if (m_benchmark) {
            fprintf(stderr, "DEPTH %fs: %f mpix/s, %f gb/s raw, %f gb/s read (cuda=%d) (ps=%d, ts=%d)\n",
                  depthdur,
                  pix/depthdur/1e6,
                  bytesraw/depthdur/(1024*1024*1024),
                  bytesread/depthdur/(1024*1024*1024),
                  0, depthps, structsz);
         }
#endif
      }
   }
}

void VncServer::invalidate(int x, int y, int w, int h) {

   rfbMarkRectAsModified(m_screen, x, y, w, h);
   m_depthSent = false;
}
