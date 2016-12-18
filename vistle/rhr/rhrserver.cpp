/**\file
 * \brief RhrServer plugin class
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
#include "rhrserver.h"

#include <tbb/parallel_for.h>
#include <tbb/concurrent_queue.h>
#include <tbb/enumerable_thread_specific.h>

#include <util/stopwatch.h>
#include <core/tcpmessage.h>

//#define QUANT_ERROR
//#define TIMING

#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>

#define CERR std::cerr << "RHR: "


namespace vistle {

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

RhrServer *RhrServer::plugin = NULL;




//! called when plugin is loaded
RhrServer::RhrServer(unsigned short port)
: m_acceptor(m_io)
, m_port(0)
{
   vassert(plugin == NULL);
   plugin = this;

   //fprintf(stderr, "new RhrServer plugin\n");

   init(port);
}

// this is called if the plugin is removed at runtime
RhrServer::~RhrServer()
{
   vassert(plugin);
   m_clientSocket.reset();

   //fprintf(stderr,"RhrServer::~RhrServer\n");

   plugin = nullptr;
}

void RhrServer::setColorCodec(ColorCodec value) {

    switch(value) {
        case Raw:
            m_imageParam.rgbaJpeg = false;
            m_imageParam.rgbaSnappy = false;
            break;
        case Jpeg_YUV411:
        case Jpeg_YUV444:
            m_imageParam.rgbaJpeg = true;
            m_imageParam.rgbaSnappy = false;
            m_imageParam.rgbaChromaSubsamp = value==Jpeg_YUV411;
            break;
        case Snappy:
            m_imageParam.rgbaJpeg = false;
            m_imageParam.rgbaSnappy = true;
            break;
    }
}

void RhrServer::enableQuantization(bool value) {

   m_imageParam.depthQuant = value;
}

void RhrServer::enableDepthSnappy(bool value) {

    m_imageParam.depthSnappy = value;
}

void RhrServer::setDepthPrecision(int bits) {

    m_imageParam.depthPrecision = bits;
}

unsigned short RhrServer::port() const {

   return m_port;
}

int RhrServer::numViews() const {

    return m_viewData.size();
}

unsigned char *RhrServer::rgba(int i) {

    return m_viewData[i].rgba.data();
}

const unsigned char *RhrServer::rgba(int i) const {

    return m_viewData[i].rgba.data();
}

float *RhrServer::depth(int i) {

    return m_viewData[i].depth.data();
}

const float *RhrServer::depth(int i) const {

    return m_viewData[i].depth.data();
}

int RhrServer::width(int i) const {

   return m_viewData[i].param.width;
}

int RhrServer::height(int i) const {

   return m_viewData[i].param.height;
}

const vistle::Matrix4 &RhrServer::viewMat(int i) const {

   return m_viewData[i].param.view;
}

const vistle::Matrix4 &RhrServer::projMat(int i) const {

   return m_viewData[i].param.proj;
}

const vistle::Matrix4 &RhrServer::modelMat(int i) const {

   return m_viewData[i].param.model;
}

void RhrServer::setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius) {

   m_boundCenter = center;
   m_boundRadius = radius;
}

//! called after plug-in is loaded and scenegraph is initialized
bool RhrServer::init(unsigned short port) {

   lightsUpdateCount = 0;

   m_appHandler = nullptr;

   m_tileWidth = 256;
   m_tileHeight = 256;

   m_numTimesteps = 0;

   m_boundCenter = vistle::Vector3(0., 0., 0.);
   m_boundRadius = 1.;

   m_delay = 0;

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

   return start(port);
}

bool RhrServer::start(unsigned short port) {

    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), port);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(acceptor::reuse_address(true));
    try {
        m_acceptor.bind(endpoint);
    } catch(const boost::system::system_error &err) {
        if (err.code() == boost::system::errc::address_in_use) {
            m_acceptor.close();
            CERR << "failed to listen on port " << port << " - address already in use" << std::endl;
            return false;
        } else {
            CERR << "listening on port " << port << " failed" << std::endl;
      }
      throw(err);
   }
   m_acceptor.listen();
   CERR << "forwarding connections on port " << port << std::endl;

   m_port = port;

   return startAccept();
}

bool RhrServer::startAccept() {

   assert(!m_clientSocket);
   auto sock = std::make_shared<asio::ip::tcp::socket>(m_io);

   m_acceptor.async_accept(*sock, [this, sock](boost::system::error_code ec){handleAccept(sock, ec);});
   return true;
}

void RhrServer::handleAccept(std::shared_ptr<asio::ip::tcp::socket> sock, const boost::system::error_code &error) {

   assert(!m_clientSocket);
   if (error) {
      CERR << "error in accept: " << error.message() << std::endl;
      return;
   }

   CERR << "incoming connection..." << std::endl;

   m_clientSocket = sock;
}


void RhrServer::setAppMessageHandler(AppMessageHandlerFunc handler) {

   m_appHandler = handler;
}

void RhrServer::resize(int viewNum, int w, int h) {

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
   }
}

void RhrServer::deferredResize() {

    assert(!m_resizeBlocked);
    for (int i=0; i<numViews(); ++i) {
        resize(i, -1, -1);
    }
    m_resizeDeferred = false;
}

//! handle matrix update message
bool RhrServer::handleMatrices(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const matricesMsg &mat) {

   size_t viewNum = mat.viewNum >= 0 ? mat.viewNum : 0;
   if (viewNum >= m_viewData.size()) {
       m_viewData.resize(viewNum+1);
   }
   
   resize(viewNum, mat.width, mat.height);

   ViewData &vd = m_viewData[viewNum];

   vd.nparam.timestep = timestep();
   vd.nparam.matrixTime = mat.time;
   vd.nparam.requestNumber = mat.requestNumber;

   for (int i=0; i<16; ++i) {
      vd.nparam.proj.data()[i] = mat.proj[i];
      vd.nparam.view.data()[i] = mat.view[i];
      vd.nparam.model.data()[i] = mat.model[i];
   }

   //std::cerr << "handleMatrices: view " << mat.viewNum << ", proj: " << vd.nparam.proj << std::endl;

   if (mat.last) {
       for (int i=0; i<numViews(); ++i) {
           m_viewData[i].param = m_viewData[i].nparam;
       }
   }

   return true;
}

//! handle light update message
bool RhrServer::handleLights(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const lightsMsg &light) {

#define SET_VEC(d, dst, src) \
      do { \
         for (int k=0; k<d; ++k) { \
            (dst)[k] = (src)[k]; \
         } \
      } while(false)

   std::vector<Light> newLights;
   for (int i=0; i<lightsMsg::NumLights; ++i) {

      const auto &cl = light.lights[i];
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

   std::swap(lights, newLights);
   if (lights != newLights) {
       ++lightsUpdateCount;
       std::cerr << "handleLightsMessage: lights changed" << std::endl;
   }

   //std::cerr << "handleLightsMessage: " << lights.size() << " lights received" << std::endl;

   return true;
}


//! send generic application message to a client
void RhrServer::sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data) {

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
void RhrServer::broadcastApplicationMessage(int type, int length, const char *data) {

   sendApplicationMessage(nullptr, type, length, data);
}

#if 0
//! handle generic application message
bool RhrServer::handleApplicationMessage(rfbClientPtr cl, void *data,
      const rfbClientToServerMsg *message) {

   if (message->type != rfbApplication)
      return FALSE;

   if (true /* TODO: how to check for new client */ )
   {
       std::cerr << "new client -> sending num timesteps: " << plugin->m_numTimesteps << std::endl;
       appAnimationTimestep app;
       app.current = plugin->m_imageParam.timestep;
       app.total = plugin->m_numTimesteps;
       sendApplicationMessage(cl, rfbAnimationTimestep, sizeof(app), (char *)&app);
   }

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
      case rfbFeedback:
      {
         // not needed: handled via vistle connection
      }
      break;
      case rfbAnimationTimestep: {
         appAnimationTimestep app;
         memcpy(&app, &buf[0], sizeof(app));
         plugin->m_imageParam.timestep = app.current;
         std::cerr << "vnc: app timestep: " << app.current << std::endl;
      }
      break;
   }

   return TRUE;
}
#endif

unsigned RhrServer::timestep() const {

   return m_imageParam.timestep;
}

void RhrServer::setNumTimesteps(unsigned num) {

   if (num != m_numTimesteps) {
      m_numTimesteps = num;
      appAnimationTimestep app;
      app.current = m_imageParam.timestep;
      app.total = num;
      broadcastApplicationMessage(rfbAnimationTimestep, sizeof(app), (char *)&app);
   }
}

//! send bounding sphere of scene to a client
void RhrServer::sendBoundsMessage(std::shared_ptr<socket> sock) {

#if 0
   std::cerr << "sending bounds: "
             << "c: " << m_boundCenter
             << "r: " << m_boundRadius << std::endl;
#endif

   boundsMsg msg;
   msg.type = rfbBounds;
   msg.center[0] = m_boundCenter[0];
   msg.center[1] = m_boundCenter[1];
   msg.center[2] = m_boundCenter[2];
   msg.radius = m_boundRadius;

   RemoteRenderMessage r(msg, 0);
   message::send(*sock, r);
}


//! handle request for a bounding sphere update
bool RhrServer::handleBounds(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const boundsMsg &bound) {

   if (bound.sendreply) {
       std::cout << "SENDING BOUNDS" << std::endl;
      sendBoundsMessage(sock);
   }

   return true;
}

bool RhrServer::handleAnimation(std::shared_ptr<RhrServer::socket> sock, const vistle::message::RemoteRenderMessage &msg, const animationMsg &anim) {

    CERR << "app timestep: " << anim.current << std::endl;
    m_imageParam.timestep = anim.current;
}

//! this is called before every frame, used for polling for RFB messages
void
RhrServer::preFrame() {

   const int wait_msec=0;

   if (m_delay) {
      usleep(m_delay);
   }

   m_io.poll();
   if (m_clientSocket) {
      std::shared_ptr<boost::asio::ip::tcp::socket> sock;
      size_t avail = 0;
      asio::socket_base::bytes_readable command(true);
      m_clientSocket->io_control(command);
      if (command.get() > 0) {
          avail = command.get();
      }
      if (avail >= sizeof(message::RemoteRenderMessage)) {
          message::Buffer msg;
          bool received = false;
          std::vector<char> payload;
          if (message::recv(*m_clientSocket, msg, received, false, &payload) && received) {
              switch(msg.type()) {
              case message::Message::REMOTERENDERING: {
                  auto &m = msg.as<message::RemoteRenderMessage>();
                  auto &rhr = m.rhr();
                  switch (rhr.type) {
                  case rfbMatrices: {
                      auto &mat = static_cast<const matricesMsg &>(rhr);
                      handleMatrices(m_clientSocket, m, mat);
                      break;
                  }
                  case rfbLights: {
                      auto &light = static_cast<const lightsMsg &>(rhr);
                      handleLights(m_clientSocket, m, light);
                      break;
                  }
                  case rfbAnimation: {
                      auto &anim = static_cast<const animationMsg &>(rhr);
                      handleAnimation(m_clientSocket, m, anim);
                      break;
                  }
                  case rfbBounds: {
                      auto &bound = static_cast<const boundsMsg &>(rhr);
                      handleBounds(m_clientSocket, m, bound);
                      break;
                  }
                  case rfbTile:
                  case rfbApplication:
                  default:
                      CERR << "invalid RHR message subtype received" << std::endl;
                      break;
                  }
                  break;
              }
              default: {
                  CERR << "invalid message type received" << std::endl;
                  break;
              }
              }
          }
          //handleClient(sock);
      }
   }
}

void RhrServer::invalidate(int viewNum, int x, int y, int w, int h, const RhrServer::ViewParameters &param, bool lastView) {

   if (m_clientSocket)
      encodeAndSend(viewNum, x, y, w, h, param, lastView);
}

namespace {

tileMsg *newTileMsg(const RhrServer::ImageParameters &param, const RhrServer::ViewParameters &vp, int viewNum, int x, int y, int w, int h) {

   assert(x+w <= vp.width);
   assert(y+h <= vp.height);

   tileMsg *message = new tileMsg;

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
   message->timestep = vp.timestep;
   for (int i=0; i<16; ++i) {
      message->model[i] = vp.model.data()[i];
      message->view[i] = vp.view.data()[i];
      message->proj[i] = vp.proj.data()[i];
   }

   return message;
}

}

struct EncodeTask: public tbb::task {

    tbb::concurrent_queue<RhrServer::EncodeResult> &resultQueue;
    int viewNum;
    int x, y, w, h, stride;
    int bpp;
    bool subsamp;
    float *depth;
    unsigned char *rgba;
    tileMsg *message;

    EncodeTask(tbb::concurrent_queue<RhrServer::EncodeResult> &resultQueue, int viewNum, int x, int y, int w, int h,
          float *depth, const RhrServer::ImageParameters &param, const RhrServer::ViewParameters &vp)
    : resultQueue(resultQueue)
    , viewNum(viewNum)
    , x(x)
    , y(y)
    , w(w)
    , h(h)
    , stride(vp.width)
    , bpp(4)
    , subsamp(false)
    , depth(depth)
    , rgba(nullptr)
    , message(nullptr)
    {
        assert(depth);
        message = newTileMsg(param, vp, viewNum, x, y, w, h);

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

    EncodeTask(tbb::concurrent_queue<RhrServer::EncodeResult> &resultQueue, int viewNum, int x, int y, int w, int h,
          unsigned char *rgba, const RhrServer::ImageParameters &param, const RhrServer::ViewParameters &vp)
    : resultQueue(resultQueue)
    , viewNum(viewNum)
    , x(x)
    , y(y)
    , w(w)
    , h(h)
    , stride(vp.width)
    , bpp(4)
    , subsamp(param.rgbaChromaSubsamp)
    , depth(nullptr)
    , rgba(rgba)
    , message(nullptr)
    {
       assert(rgba);
       message = newTileMsg(param, vp, viewNum, x, y, w, h);
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

    tbb::task* execute() {

        auto &msg = *message;
        RhrServer::EncodeResult result(message);
        if (depth) {
            const char *zbuf = reinterpret_cast<const char *>(depth);
            if (msg.compression & rfbTileDepthQuantize) {
                const int ds = msg.format == rfbDepth16Bit ? 2 : 3;
                msg.size = depthquant_size(DepthFloat, ds, w, h);
                std::vector<char> qbuf(msg.size);
                depthquant(qbuf.data(), zbuf, DepthFloat, ds, x, y, w, h, stride);
#ifdef QUANT_ERROR
                std::vector<char> dequant(sizeof(float)*w*h);
                depthdequant(dequant.data(), qbuf, DepthFloat, ds, 0, 0, w, h);
                //depthquant(qbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride); // test depthcompare
                depthcompare(zbuf, dequant.data(), DepthFloat, ds, x, y, w, h, stride);
#endif
                result.payload = std::move(qbuf);
            } else {
                std::vector<char> tilebuf(msg.size);
                for (int yy=0; yy<h; ++yy) {
                    memcpy(tilebuf.data()+yy*bpp*w, zbuf+((yy+y)*stride+x)*bpp, w*bpp);
                }
                result.payload = std::move(tilebuf);
            }
        } else if (rgba) {
            if (msg.compression & rfbTileJpeg) {
                int ret = -1;
#ifdef HAVE_TURBOJPEG
                TJSAMP sampling = subsamp ? TJSAMP_420 : TJSAMP_444;
                TjContext::reference tj = tjContexts.local();
                size_t maxsize = tjBufSize(msg.width, msg.height, sampling);
                std::vector<char> jpegbuf(maxsize);
                unsigned long sz = 0;
                //unsigned char *src = reinterpret_cast<unsigned char *>(rgba);
                rgba += (msg.totalwidth*msg.y+msg.x)*bpp;
                {
#ifdef TIMING
                   double start = vistle::Clock::time();
#endif
                   ret = tjCompress(tj.handle, rgba, msg.width, msg.totalwidth*bpp, msg.height, bpp, reinterpret_cast<unsigned char *>(jpegbuf.data()), &sz, subsamp, 90, TJ_BGR);
#ifdef TIMING
                   double dur = vistle::Clock::time() - start;
                   std::cerr << "JPEG compression: " << dur << "s, " << msg.width*(msg.height/dur)/1e6 << " MPix/s" << std::endl;
#endif
                }
                if (ret >= 0) {
                    msg.size = sz;
                    result.payload = std::move(jpegbuf);
                }
#endif
                if (ret < 0)
                    msg.compression &= ~rfbTileJpeg;
            }
            if (!(msg.compression & rfbTileJpeg)) {
                std::vector<char> tilebuf(msg.size);
                for (int yy=0; yy<h; ++yy) {
                    memcpy(tilebuf.data()+yy*bpp*w, rgba+((yy+y)*stride+x)*bpp, w*bpp);
                }
                result.payload = std::move(tilebuf);
            }
        }
#ifdef HAVE_SNAPPY
        if((msg.compression & rfbTileSnappy) && !(msg.compression & rfbTileJpeg)) {
            size_t maxsize = snappy::MaxCompressedLength(msg.size);
            std::vector<char> sbuf(maxsize);
            size_t compressed = 0;
            { 
#ifdef TIMING
               double start = vistle::Clock::time();
#endif
               snappy::RawCompress(result.payload.data(), msg.size, sbuf.data(), &compressed);
#ifdef TIMING
               vistle::StopWatch timer(rgba ? "snappy RGBA" : "snappy depth");
               double dur = vistle::Clock::time() - start;
               std::cerr << "SNAPPY " << (rgba ? "RGB" : "depth") << ": " << dur << "s, " << msg.width*(msg.height/dur)/1e6 << " MPix/s" << std::endl;
#endif
            }
            msg.size = compressed;

            //std::cerr << "compressed " << msg.size << " to " << compressed << " (buf: " << cd->buf.size() << ")" << std::endl;
            result.payload = std::move(sbuf);
        } else {
            msg.compression &= ~rfbTileSnappy;
        }
#endif
        assert(result.payload.size() == msg.size);
        resultQueue.push(result);
        return nullptr; // or a pointer to a new task to be executed immediately
    }
};

void RhrServer::setTileSize(int w, int h) {

   m_tileWidth = w;
   m_tileHeight = h;
}

void RhrServer::encodeAndSend(int viewNum, int x0, int y0, int w, int h, const RhrServer::ViewParameters &param, bool lastView) {

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
    }

    bool tileReady = false;
    do {
        RhrServer::EncodeResult result;
        tileReady = false;
        tileMsg *msg = nullptr;
        if (m_queuedTiles == 0 && lastView) {
           msg = newTileMsg(m_imageParam, param, viewNum, x0, y0, w, h);
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

           message::RemoteRenderMessage vmsg(*msg, result.payload.size());
           message::send(*m_clientSocket, vmsg, &result.payload);
        }
        result.payload.clear();
        delete msg;
    } while (m_queuedTiles > 0 && (tileReady || lastView));

    if (lastView) {
        vassert(m_queuedTiles == 0);
        m_resizeBlocked = false;
        deferredResize();
    }
}

RhrServer::ViewParameters RhrServer::getViewParameters(int viewNum) const {

    return m_viewData[viewNum].param;
}

} // namespace vistle
