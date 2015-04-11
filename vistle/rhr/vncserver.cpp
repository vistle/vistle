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

#include "rfbext.h"
#include "depthquant.h"
#include "vncserver.h"

#include <tbb/parallel_for.h>
#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>

#include <util/stopwatch.h>

//#define QUANT_ERROR
//#define TIMING

#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>

struct TjComp {

       TjComp()
                 : handle(tjInitCompress())
                       {}

          ~TjComp() {
                    tjDestroy(handle);
                       }

             tjhandle handle;
};

typedef tbb::enumerable_thread_specific<TjComp> TjContext;
static TjContext tjContexts;
#endif

VncServer *VncServer::plugin = NULL;

//! per-client data: supported extensions
struct ClientData {
   ClientData()
      : supportsBounds(false)
      , supportsTile(false)
      , tileCompressions(0)
      , supportsApplication(false)
      , supportsLights(false)
   {
   }
   bool supportsBounds;
   uint8_t depthCompressions;
   bool supportsTile;
   uint8_t tileCompressions;
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

static rfbProtocolExtension tileExt = {
   NULL, // newClient
   NULL, // init
   tileEncodings, // pseudoEncodings
   VncServer::enableTile, // enablePseudoEncoding
   VncServer::handleTileMessage, // handleMessage
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

//! called when plugin is loaded
VncServer::VncServer(int w, int h, const std::string &host, unsigned short port) {
   vassert(plugin == NULL);
   plugin = this;

   //fprintf(stderr, "new VncServer plugin\n");

   init(w, h, port);

   Client c(host, port);
   m_clientList.push_back(c);
}

// this is called if the plugin is removed at runtime
VncServer::~VncServer()
{
   vassert(plugin);

   rfbShutdownServer(m_screen, true);
   rfbScreenCleanup(m_screen);
   m_screen = nullptr;

   //fprintf(stderr,"VncServer::~VncServer\n");

   plugin = nullptr;
}

void VncServer::setColorCodec(ColorCodec value) {

    switch(value) {
        case Raw:
            m_imageParam.rgbaJpeg = false;
            m_imageParam.rgbaSnappy = false;
            break;
        case Jpeg:
            m_imageParam.rgbaJpeg = true;
            m_imageParam.rgbaSnappy = false;
            break;
        case Snappy:
            m_imageParam.rgbaJpeg = false;
            m_imageParam.rgbaSnappy = true;
            break;
    }
}

void VncServer::enableQuantization(bool value) {

   m_imageParam.depthQuant = value;
}

void VncServer::enableDepthSnappy(bool value) {

    m_imageParam.depthSnappy = value;
}

void VncServer::setDepthPrecision(int bits) {

    m_imageParam.depthPrecision = bits;
}

unsigned short VncServer::port() const {

   if (m_clientList.empty())
      return m_screen->port;
   else
      return m_clientList.front().port;
}

std::string VncServer::host() const {

   if (m_clientList.empty())
      return "";
   else
      return m_clientList.front().host;
}

int VncServer::numViews() const {

    return m_viewData.size();
}

unsigned char *VncServer::rgba(int i) {

    return m_viewData[i].rgba.data();
}

const unsigned char *VncServer::rgba(int i) const {

    return m_viewData[i].rgba.data();
}

float *VncServer::depth(int i) {

    return m_viewData[i].depth.data();
}

const float *VncServer::depth(int i) const {

    return m_viewData[i].depth.data();
}

int VncServer::width(int i) const {

   return m_viewData[i].param.width;
}

int VncServer::height(int i) const {

   return m_viewData[i].param.height;
}

const vistle::Matrix4 &VncServer::viewMat(int i) const {

   return m_viewData[i].param.view;
}

const vistle::Matrix4 &VncServer::projMat(int i) const {

   return m_viewData[i].param.proj;
}

const vistle::Matrix4 &VncServer::modelMat(int i) const {

   return m_viewData[i].param.model;
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

   lightsUpdateCount = 0;

   m_appHandler = nullptr;

   m_tileWidth = 256;
   m_tileHeight = 256;

   m_numTimesteps = 0;

   m_numClients = 0;
   m_numRhrClients = 0;
   m_boundCenter = vistle::Vector3(0., 0., 0.);
   m_boundRadius = 1.;

   rfbRegisterProtocolExtension(&matricesExt);
   rfbRegisterProtocolExtension(&lightsExt);
   rfbRegisterProtocolExtension(&boundsExt);
   rfbRegisterProtocolExtension(&tileExt);
   rfbRegisterProtocolExtension(&applicationExt);

   m_delay = 0;
   std::string config("COVER.Plugin.VncServer");

   m_benchmark = false;
   m_errormetric = false;
   m_compressionrate = false;

   m_imageParam.depthPrecision = 32;
   m_imageParam.depthQuant = true;
   m_imageParam.depthSnappy = true;
   m_imageParam.rgbaSnappy = true;
   m_imageParam.depthFloat = true;

   m_resizeBlocked = false;
   m_resizeDeferred = false;
   m_queuedTiles = 0;
   m_firstTile = false;

   int argc = 1;
   char *argv[] = { (char *)"DisCOVERay", NULL };

   m_screen = rfbGetScreen(&argc, argv, 0, 0, 8, 3, 4);
   m_screen->desktopName = "DisCOVERay";

   m_screen->autoPort = FALSE;
   m_screen->port = port;
   m_screen->ipv6port = port;

   m_screen->kbdAddEvent = &keyEvent;
   m_screen->ptrAddEvent = &pointerEvent;
   m_screen->newClientHook = &newClientHook;

   m_screen->deferUpdateTime = 0;
   m_screen->maxRectsPerUpdate = 10000000;
   rfbInitServer(m_screen);
   m_screen->deferUpdateTime = 0;
   m_screen->maxRectsPerUpdate = 10000000;
   m_screen->handleEventsEagerly = 1;

   m_screen->cursor = NULL; // don't show a cursor

   resize(0, w, h);

   return true;
}

void VncServer::setAppMessageHandler(AppMessageHandlerFunc handler) {

   m_appHandler = handler;
}

int VncServer::numClients() const {

   return m_numClients;
}

int VncServer::numRhrClients() const {

   return m_numRhrClients;
}

void VncServer::resize(int viewNum, int w, int h) {

#if 0
    if (w!=-1 && h!=-1) {
        std::cout << "resize: view=" << viewNum << ", w=" << w << ", h=" << h << std::endl;
    }
#endif
    if (viewNum >= numViews()) {
        if (viewNum != 0)
            m_viewData.emplace_back();
        else
            return;
    }

   ViewData &vd = m_viewData[viewNum];
   if (m_resizeBlocked) {
       m_resizeDeferred = true;

       vd.newWidth = w;
       vd.newHeight = h;
       return;
   }

   if (w==-1 && h==-1) {

       w = vd.newWidth;
       h = vd.newHeight;
   }

   if (vd.nparam.width != w || vd.nparam.height != h) {
      vd.nparam.width = w;
      vd.nparam.height = h;

      vd.rgba.resize(w*h*4);
      vd.depth.resize(w*h);

      if (viewNum == 0) {
          m_screen->frameBuffer = reinterpret_cast<char *>(vd.rgba.data());
          rfbNewFramebuffer(m_screen, m_screen->frameBuffer,
                  w, h, 8, 3, 4);
      }
   }
}

void VncServer::deferredResize() {

    assert(!m_resizeBlocked);
    for (int i=0; i<numViews(); ++i) {
        resize(i, -1, -1);
    }
    m_resizeDeferred = false;
}

//! count connected clients
enum rfbNewClientAction VncServer::newClientHook(rfbClientPtr cl) {

   ++plugin->m_numClients;
   cl->clientGoneHook = clientGoneHook;

   return RFB_CLIENT_ACCEPT;
}


//! clean up per-client data when client disconnects
void VncServer::clientGoneHook(rfbClientPtr cl) {

   --plugin->m_numClients;

   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (cd->supportsTile) {
      --plugin->m_numRhrClients;
      std::cerr << "VncServer: RHR client gone (#RHR: " << plugin->m_numClients << ", #total: " << plugin->m_numClients << ")" << std::endl;
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

   size_t viewNum = msg.viewNum >= 0 ? msg.viewNum : 0;
   if (viewNum >= plugin->m_viewData.size()) {
       plugin->m_viewData.resize(viewNum+1);
   }
   
   plugin->resize(viewNum, msg.width, msg.height);

   ViewData &vd = plugin->m_viewData[viewNum];

   vd.nparam.matrixTime = msg.time;
   vd.nparam.requestNumber = msg.requestNumber;

   for (int i=0; i<16; ++i) {
      vd.nparam.proj.data()[i] = msg.proj[i];
      vd.nparam.view.data()[i] = msg.view[i];
      vd.nparam.model.data()[i] = msg.model[i];
   }

   //std::cerr << "handleMatrices: view " << msg.viewNum << ", proj: " << vd.nparam.proj << std::endl;

   if (msg.last) {
       for (int i=0; i<plugin->numViews(); ++i) {
           plugin->m_viewData[i].param = plugin->m_viewData[i].nparam;
       }
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
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   cd->supportsLights = true;

#define SET_VEC(d, dst, src) \
      do { \
         for (int k=0; k<d; ++k) { \
            (dst)[k] = (src)[k]; \
         } \
      } while(false)

   std::vector<Light> newLights;
   for (int i=0; i<lightsMsg::NumLights; ++i) {

      const auto &cl = msg.lights[i];
      newLights.emplace_back();
      auto &l = newLights.back();

      SET_VEC(4, l.position, cl.position);
      SET_VEC(4, l.ambient, cl.ambient);
      SET_VEC(4, l.diffuse, cl.diffuse);
      SET_VEC(4, l.specular, cl.specular);
      SET_VEC(3, l.attenuation, cl.attenuation);
      SET_VEC(3, l.direction, cl.spot_direction);
      l.spotCutoff = cl.spot_cutoff;
      l.spotExponent = cl.spot_exponent;
      l.enabled = cl.enabled;
      //std::cerr << "Light " << i << ": ambient: " << l.ambient << std::endl;
      //std::cerr << "Light " << i << ": diffuse: " << l.diffuse << std::endl;
   }

   std::swap(plugin->lights, newLights);
   if (plugin->lights != newLights) {
       ++plugin->lightsUpdateCount;
       std::cerr << "handleLightsMessage: lights changed" << std::endl;
   }

   //std::cerr << "handleLightsMessage: " << plugin->lights.size() << " lights received" << std::endl;

   return TRUE;
}

//! enable sending of image tiles
rfbBool VncServer::enableTile(rfbClientPtr cl, void **data, int encoding) {

   if (encoding != rfbTile)
      return FALSE;

   tileMsg msg;
   if (rfbWriteExact(cl, (char *)&msg, sizeof(msg)) < 0) {
         rfbLogPerror("enableTile: write");
   }

   return TRUE;
}

//! handle image tile request by client
rfbBool VncServer::handleTileMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbTile)
      return FALSE;

   if (!cl->clientData) {
      cl->clientData = new ClientData();
   }
   ClientData *cd = static_cast<ClientData *>(cl->clientData);
   if (!cd->supportsTile) {
      ++plugin->m_numRhrClients;
      std::cerr << "VncServer: RHR client connected (#RHR: " << plugin->m_numClients << ", #total: " << plugin->m_numClients << ")" << std::endl;
   }
   cd->supportsTile = true;

   tileMsg msg;
   int n = rfbReadExact(cl, ((char *)&msg)+1, sizeof(msg)-1);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleTileMessage: read");
      rfbCloseClient(cl);
      return TRUE;
   }
   cd->tileCompressions = msg.compression;
   std::vector<char> buf(msg.size);
   n = rfbReadExact(cl, &buf[0], msg.size);
   if (n <= 0) {
      if (n!= 0)
         rfbLogPerror("handleTileMessage: read data");
      rfbCloseClient(cl);
      return TRUE;
   }

   if (msg.flags & rfbTileRequest) {
       std::cerr << "VncServer: tile request ignored" << std::endl;
      // FIXME
      //sendDepthMessage(cl);
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

      appAnimationTimestep app;
      app.current = plugin->m_imageParam.timestep;
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

   if (plugin->m_appHandler) {
      bool handled = plugin->m_appHandler(msg.appType, buf);
      if (handled)
         return TRUE;
   }

   switch (msg.appType) {
      case rfbScreenConfig:
      {
         appScreenConfig app;
         memcpy(&app, &buf[0], sizeof(app));
         plugin->resize(0, app.width, app.height);
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
         plugin->m_imageParam.timestep = app.current;
      }
      break;
   }

   return TRUE;
}

unsigned VncServer::timestep() const {

   return m_imageParam.timestep;
}

void VncServer::setNumTimesteps(unsigned num) {

   if (num != m_numTimesteps) {
      m_numTimesteps = num;
      appAnimationTimestep app;
      app.current = m_imageParam.timestep;
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
       std::cout << "SENDING BOUNDS" << std::endl;
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

   if (m_numClients == 0) {
      for (size_t i=0; i<m_clientList.size(); ++i) {
         if (rfbReverseConnection(m_screen, const_cast<char *>(m_clientList[i].host.c_str()), m_clientList[i].port)) {
            break;
         }
      }
   }

   rfbCheckFds(m_screen, wait_msec*1000);
   rfbHttpCheckFds(m_screen);

   rfbClientIteratorPtr i = rfbGetClientIterator(m_screen);
   while (rfbClientPtr cl = rfbClientIteratorNext(i)) {
      if (rfbUpdateClient(cl)) {
      }
   }
   rfbReleaseClientIterator(i);
}

void VncServer::invalidate(int viewNum, int x, int y, int w, int h, const VncServer::ViewParameters &param, bool lastView) {

    if (m_numClients - m_numRhrClients > 0) {
        std::cerr << "Non-RHR clients: " << m_numClients - m_numRhrClients << std::endl;
        rfbMarkRectAsModified(m_screen, x, y, w, h);
    }

    if (m_numRhrClients > 0) {
        encodeAndSend(viewNum, x, y, w, h, param, lastView);
    }
}

struct EncodeTask: public tbb::task {

    tbb::concurrent_queue<VncServer::EncodeResult> &resultQueue;
    int viewNum;
    int x, y, w, h, stride;
    int bpp;
    float *depth;
    unsigned char *rgba;
    tileMsg *message;

    EncodeTask(tbb::concurrent_queue<VncServer::EncodeResult> &resultQueue, int viewNum, int x, int y, int w, int h,
          float *depth, const VncServer::ImageParameters &param, const VncServer::ViewParameters &vp)
    : resultQueue(resultQueue)
    , viewNum(viewNum)
    , x(x)
    , y(y)
    , w(w)
    , h(h)
    , stride(vp.width)
    , bpp(4)
    , depth(depth)
    , rgba(nullptr)
    , message(nullptr)
    {
        assert(depth);
        initMsg(param, vp, viewNum, x, y, w, h);

        message->size = message->width * message->height;
        if (param.depthFloat) {
            message->format = rfbDepthFloat;
            bpp = 4;
        } else {
            switch(param.depthPrecision) {
                case 8:
                    message->format = rfbDepth8Bit;
                    bpp = 1;
                    break;
                case 16:
                    message->format = rfbDepth16Bit;
                    bpp = 2;
                    break;
                case 24:
                    message->format = rfbDepth24Bit;
                    bpp = 4;
                    break;
                case 32:
                    message->format = rfbDepth32Bit;
                    bpp = 4;
                    break;
            }
        }

        message->size = bpp * message->width * message->height;

        if (param.depthQuant) {
            message->format = param.depthPrecision<=16 ? rfbDepth16Bit : rfbDepth24Bit;
            message->compression |= rfbTileDepthQuantize;
        }
#ifdef HAVE_SNAPPY
        if (param.depthSnappy) {
            message->compression |= rfbTileSnappy;
        }
#endif
    }

    EncodeTask(tbb::concurrent_queue<VncServer::EncodeResult> &resultQueue, int viewNum, int x, int y, int w, int h,
          unsigned char *rgba, const VncServer::ImageParameters &param, const VncServer::ViewParameters &vp)
    : resultQueue(resultQueue)
    , viewNum(viewNum)
    , x(x)
    , y(y)
    , w(w)
    , h(h)
    , stride(vp.width)
    , bpp(4)
    , depth(nullptr)
    , rgba(rgba)
    , message(nullptr)
    {
       assert(rgba);
       initMsg(param, vp, viewNum, x, y, w, h);
       message->size = message->width * message->height * bpp;
       message->format = rfbColorRGBA;

        if (param.rgbaJpeg) {
            message->compression |= rfbTileJpeg;
#ifdef HAVE_SNAPPY
        } else if (param.rgbaSnappy) {
            message->compression |= rfbTileSnappy;
#endif
        }
    }

    void initMsg(const VncServer::ImageParameters &param, const VncServer::ViewParameters &vp, int viewNum, int x, int y, int w, int h) {

        assert(x+w <= vp.width);
        assert(y+h <= vp.height);
        assert(stride == vp.width);

        assert(message == nullptr);

        message = new tileMsg;
        message->viewNum = viewNum;
        message->width = w;
        message->height = h;
        message->x = x;
        message->y = y;
        message->totalwidth = vp.width;
        message->totalheight = vp.height;
        message->size = 0;
        message->compression = rfbTileRaw;

        message->frameNumber = vp.frameNumber;
        message->requestNumber = vp.requestNumber;
        message->requestTime = vp.matrixTime;
        for (int i=0; i<16; ++i) {
            message->model[i] = vp.model.data()[i];
            message->view[i] = vp.view.data()[i];
            message->proj[i] = vp.proj.data()[i];
        }
    }

    tbb::task* execute() {

        auto &msg = *message;
        VncServer::EncodeResult result(message);
        if (depth) {
            const char *zbuf = reinterpret_cast<const char *>(depth);
            if (msg.compression & rfbTileDepthQuantize) {
                const int ds = msg.format == rfbDepth16Bit ? 2 : 3;
                msg.size = depthquant_size(DepthFloat, ds, w, h);
                char *qbuf = new char[msg.size];
                depthquant(qbuf, zbuf, DepthFloat, ds, x, y, w, h, stride);
#ifdef QUANT_ERROR
                std::vector<char> dequant(sizeof(float)*w*h);
                depthdequant(dequant.data(), qbuf, DepthFloat, ds, 0, 0, w, h);
                //depthquant(qbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride); // test depthcompare
                depthcompare(zbuf, dequant.data(), DepthFloat, ds, w, h);
#endif
                result.payload = qbuf;
            } else {
                char *tilebuf = new char[msg.size];
                for (int yy=0; yy<h; ++yy) {
                    memcpy(tilebuf+yy*bpp*w, zbuf+((yy+y)*stride+x)*bpp, w*bpp);
                }
                result.payload = tilebuf;
            }
        } else if (rgba) {
            if (msg.compression & rfbTileJpeg) {
                int ret = -1;
#ifdef HAVE_TURBOJPEG
                TJSAMP subsamp = TJSAMP_420;
                TjContext::reference tj = tjContexts.local();
                size_t maxsize = tjBufSize(msg.width, msg.height, subsamp);
                char *jpegbuf = new char[maxsize];
                unsigned long sz = 0;
                //unsigned char *src = reinterpret_cast<unsigned char *>(rgba);
                rgba += (msg.totalwidth*msg.y+msg.x)*bpp;
                {
#ifdef TIMING
                   double start = vistle::Clock::time();
#endif
                   ret = tjCompress(tj.handle, rgba, msg.width, msg.totalwidth*bpp, msg.height, bpp, reinterpret_cast<unsigned char *>(jpegbuf), &sz, subsamp, 90, TJ_BGR);
#ifdef TIMING
                   double dur = vistle::Clock::time() - start;
                   std::cerr << "JPEG compression: " << dur << "s, " << msg.width*(msg.height/dur)/1e6 << " MPix/s" << std::endl;
#endif
                }
                if (ret >= 0) {
                    msg.size = sz;
                    result.payload = jpegbuf;
                }
#endif
                if (ret < 0)
                    msg.compression &= ~rfbTileJpeg;
            }
            if (!(msg.compression & rfbTileJpeg)) {
                char *tilebuf = new char[msg.size];
                for (int yy=0; yy<h; ++yy) {
                    memcpy(tilebuf+yy*bpp*w, rgba+((yy+y)*stride+x)*bpp, w*bpp);
                }
                result.payload = tilebuf;
            }
        }
#ifdef HAVE_SNAPPY
        if((msg.compression & rfbTileSnappy) && !(msg.compression & rfbTileJpeg)) {
            size_t maxsize = snappy::MaxCompressedLength(msg.size);
            char *sbuf = new char[maxsize];
            size_t compressed = 0;
            { 
#ifdef TIMING
               double start = vistle::Clock::time();
#endif
               snappy::RawCompress(result.payload, msg.size, sbuf, &compressed);
#ifdef TIMING
               vistle::StopWatch timer(rgba ? "snappy RGBA" : "snappy depth");
               double dur = vistle::Clock::time() - start;
               std::cerr << "SNAPPY " << (rgba ? "RGB" : "depth") << ": " << dur << "s, " << msg.width*(msg.height/dur)/1e6 << " MPix/s" << std::endl;
#endif
            }
            msg.size = compressed;

            //std::cerr << "compressed " << msg.size << " to " << compressed << " (buf: " << cd->buf.size() << ")" << std::endl;
            delete[] result.payload;
            result.payload = sbuf;
        } else {
            msg.compression &= ~rfbTileSnappy;
        }
#endif
        resultQueue.push(result);
        return nullptr; // or a pointer to a new task to be executed immediately
    }
};

void VncServer::setTileSize(int w, int h) {

   m_tileWidth = w;
   m_tileHeight = h;
}

void VncServer::encodeAndSend(int viewNum, int x0, int y0, int w, int h, const VncServer::ViewParameters &param, bool lastView) {

    //std::cerr << "encodeAndSend: view=" << viewNum << ", c=" << (void *)rgba(viewNum) << ", d=" << depth(viewNum) << std::endl;
    if (!m_resizeBlocked) {
        m_firstTile = true;
    }
    m_resizeBlocked = true;

    //vistle::StopWatch timer("encodeAndSend");
    const int tileWidth = m_tileWidth, tileHeight = m_tileHeight;
    static int framecount=0;
    ++framecount;

    if (viewNum >= 0) {
       for (int y=y0; y<y0+h; y+=tileHeight) {
          for (int x=x0; x<x0+w; x+=tileWidth) {

             // depth
             auto dt = new(tbb::task::allocate_root()) EncodeTask(m_resultQueue,
                   viewNum,
                   x, y,
                   std::min(tileWidth, x0+w-x),
                   std::min(tileHeight, y0+h-y),
                   depth(viewNum), m_imageParam, param);
             tbb::task::enqueue(*dt);
             ++m_queuedTiles;

             // color
             auto ct = new(tbb::task::allocate_root()) EncodeTask(m_resultQueue,
                   viewNum,
                   x, y,
                   std::min(tileWidth, x0+w-x),
                   std::min(tileHeight, y0+h-y),
                   rgba(viewNum), m_imageParam, param);
             tbb::task::enqueue(*ct);
             ++m_queuedTiles;
          }
       }
    } else if (m_queuedTiles == 0) {
       tileMsg msg;
    }

    bool tileReady = false;
    do {
        VncServer::EncodeResult result;
        tileReady = false;
        tileMsg *msg = nullptr;
        if (m_queuedTiles == 0 && lastView) {
           msg = new tileMsg;
        } else if (m_resultQueue.try_pop(result)) {
            --m_queuedTiles;
            tileReady = true;
            msg = result.message;
        }
        if (msg) {
           if (m_firstTile) {
              msg->flags |= rfbTileFirst;
              //std::cerr << "first tile: req=" << msg.requestNumber << std::endl;
           }
           m_firstTile = false;
           if (m_queuedTiles == 0 && lastView) {
              msg->flags |= rfbTileLast;
              //std::cerr << "last tile: req=" << msg.requestNumber << std::endl;
           }
           msg->frameNumber = framecount;

           rfbCheckFds(m_screen, 0);
           rfbHttpCheckFds(m_screen);
           rfbClientIteratorPtr i = rfbGetClientIterator(m_screen);
           while (rfbClientPtr cl = rfbClientIteratorNext(i)) {
              if (cl->clientData) {
                 rfbUpdateClient(cl);
                 if (rfbWriteExact(cl, (char *)msg, sizeof(*msg)) < 0) {
                    rfbLogPerror("sendTileMessage: write header");
                 }
                 if (result.payload && msg->size > 0) {
                    if (rfbWriteExact(cl, result.payload, msg->size) < 0) {
                       rfbLogPerror("sendTileMessage: write paylod");
                    }
                 }
              }
              rfbUpdateClient(cl);
           }
           rfbReleaseClientIterator(i);
        }
        delete[] result.payload;
        delete msg;
    } while (m_queuedTiles > 0 && (tileReady || lastView));

    if (lastView) {
        vassert(m_queuedTiles == 0);
        m_resizeBlocked = false;
        deferredResize();
    }
    //sleep(1);
}

VncServer::ViewParameters VncServer::getViewParameters(int viewNum) const {

    return m_viewData[viewNum].param;
}
