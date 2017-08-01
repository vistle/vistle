/**\file
 * \brief RhrServer plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014, 2015, 2016, 2017 HLRS
 *
 * \copyright GPL2+
 */

#include <iostream>
#include <core/assert.h>
#include <cmath>

#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

#ifdef HAVE_ZFP
#include <zfp.h>
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

#define CERR std::cerr << "RHR: "

#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>


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

}
#endif

namespace vistle {

template<class Message>
bool RhrServer::send(const Message &message, const std::vector<char> *payload) {

    if (m_clientSocket && !m_clientSocket->is_open()) {
        resetClient();
        CERR << "client disconnected" << std::endl;
    }

    if (!m_clientSocket)
        return false;

    RemoteRenderMessage r(message, payload ? payload->size() : 0);
    if (!message::send(*m_clientSocket, r, payload)) {
        CERR << "client error, disconnecting" << std::endl;
        resetClient();
        return false;
    }
    return true;
}

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

void RhrServer::enableDepthZfp(bool value) {

    m_imageParam.depthZfp = value;
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

void RhrServer::setZfpMode(ZfpMode mode) {

    m_imageParam.depthZfpMode = mode;
}

unsigned short RhrServer::port() const {

    return m_port;
}

asio::ip::address RhrServer::listenAddress() const {

    return m_listenAddress;
}

size_t RhrServer::numViews() const {

    return m_viewData.size();
}

unsigned char *RhrServer::rgba(size_t i) {

    if (i > numViews())
        return nullptr;
    return m_viewData[i].rgba.data();
}

const unsigned char *RhrServer::rgba(size_t i) const {

    if (i > numViews())
        return nullptr;
    return m_viewData[i].rgba.data();
}

float *RhrServer::depth(size_t i) {

    if (i > numViews())
        return nullptr;
    return m_viewData[i].depth.data();
}

const float *RhrServer::depth(size_t i) const {

    if (i > numViews())
        return nullptr;
    return m_viewData[i].depth.data();
}

int RhrServer::width(size_t i) const {

   if (i > numViews())
       return 0;
   return m_viewData[i].param.width;
}

int RhrServer::height(size_t i) const {

   if (i > numViews())
       return 0;
   return m_viewData[i].param.height;
}

const vistle::Matrix4 &RhrServer::viewMat(size_t i) const {

   return m_viewData[i].param.view;
}

const vistle::Matrix4 &RhrServer::projMat(size_t i) const {

   return m_viewData[i].param.proj;
}

const vistle::Matrix4 &RhrServer::modelMat(size_t i) const {

   return m_viewData[i].param.model;
}

void RhrServer::setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius) {

   m_boundCenter = center;
   m_boundRadius = radius;
}

void RhrServer::updateVariants(const std::vector<std::pair<std::string, vistle::RenderObject::InitialVariantVisibility>> &added, const std::vector<std::string> &removed) {

    for (const auto &var: removed) {
        auto it = m_localVariants.find(var);
        if (it != m_localVariants.end())
            m_localVariants.erase(it);
        if (m_clientSocket) {
            variantMsg msg;
            strncpy(msg.name, var.c_str(), sizeof(msg.name));
            msg.remove = 1;
            send(msg);
        }
    }

    for (const auto &var: added) {
        auto it = m_localVariants.find(var.first);
        if (it == m_localVariants.end()) {
            m_localVariants.emplace(var.first, var.second);
        }
        if (m_clientSocket) {
            variantMsg msg;
            strncpy(msg.name, var.first.c_str(), sizeof(msg.name));
            if (var.second != vistle::RenderObject::DontChange) {
                msg.configureVisibility = 1;
                msg.visible = var.second==vistle::RenderObject::Visible ? 1 : 0;
            }
            send(msg);
        }
    }
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
   m_imageParam.depthZfp = true;
   m_imageParam.depthQuant = true;
   m_imageParam.depthSnappy = true;
   m_imageParam.rgbaSnappy = true;
   m_imageParam.depthFloat = true;
   m_imageParam.depthZfpMode = ZfpFixedRate;

   m_resizeBlocked = false;
   m_resizeDeferred = false;
   m_queuedTiles = 0;
   m_firstTile = false;

   resetClient();

   return start(port);
}

void RhrServer::resetClient() {

    finishTiles(RhrServer::ViewParameters(), true /* finish */, false /* send */);

    ++m_updateCount;
    ++lightsUpdateCount;
    m_clientSocket.reset();
    lightsUpdateCount = 0;
    m_clientVariants.clear();
    m_viewData.clear();
}

bool RhrServer::start(unsigned short port) {

    for (;;) {
       asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v6(), port);
        try {
           m_acceptor.open(endpoint.protocol());
        } catch (const boost::system::system_error &err) {
           if (err.code() == boost::system::errc::address_family_not_supported) {
              CERR << "IPv6 not supported, retrying listening on port " << port << " with IPv4" << std::endl;
              endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
              m_acceptor.open(endpoint.protocol());
           } else {
              throw(err);
           }
        }
        m_acceptor.set_option(acceptor::reuse_address(true));
        try {
            m_acceptor.bind(endpoint);
        } catch(const boost::system::system_error &err) {
            if (err.code() == boost::system::errc::address_in_use) {
                m_acceptor.close();
                CERR << "failed to listen on port " << port << " - address already in use" << std::endl;
                ++port;
                continue;
            } else {
                CERR << "listening on port " << port << " failed" << std::endl;
            }
            throw(err);
        }
        m_listenAddress = endpoint.address();
        break;
    }
    m_acceptor.listen();
    CERR << "listening for connections on port " << port << std::endl;

    m_port = port;

    return startAccept();
}

bool RhrServer::startAccept() {

   auto sock = std::make_shared<asio::ip::tcp::socket>(m_io);

   m_acceptor.async_accept(*sock, [this, sock](boost::system::error_code ec){handleAccept(sock, ec);});
   return true;
}

void RhrServer::handleAccept(std::shared_ptr<asio::ip::tcp::socket> sock, const boost::system::error_code &error) {

   if (error) {
      CERR << "error in accept: " << error.message() << std::endl;
      return;
   }

   if (m_clientSocket) {
       CERR << "incoming connection, rejecting as already servicing a client" << std::endl;
       startAccept();
       return;
   }

   CERR << "incoming connection, accepting new client" << std::endl;
   m_clientSocket = sock;

   int nt = m_numTimesteps;
   ++m_numTimesteps;
   setNumTimesteps(nt);

   for (auto &var: m_localVariants) {
        variantMsg msg;
        strncpy(msg.name, var.first.c_str(), sizeof(msg.name));
        msg.visible = var.second;
        send(msg);

   }

   startAccept();
}


void RhrServer::setAppMessageHandler(AppMessageHandlerFunc handler) {

   m_appHandler = handler;
}

void RhrServer::resize(size_t viewNum, int w, int h) {

#if 0
    if (w!=-1 && h!=-1) {
        std::cout << "resize: view=" << viewNum << ", w=" << w << ", h=" << h << std::endl;
    }
#endif
    while (viewNum >= numViews()) {
        m_viewData.emplace_back();
    }

   ViewData &vd = m_viewData[viewNum];
   if (m_resizeBlocked) {

       if (w != -1 && h != -1) {
           m_resizeDeferred = true;
           vd.newWidth = w;
           vd.newHeight = h;
       }
       return;
   }

   if (w==-1 && h==-1) {

       w = vd.newWidth;
       h = vd.newHeight;
   }

   if (w == -1 || h == -1) {
      //CERR << "rejecting resize for view " << viewNum << " to " << w << "x" << h << std::endl;
      return;
   }

   if (vd.nparam.width != w || vd.nparam.height != h) {
      vd.nparam.width = w;
      vd.nparam.height = h;

      CERR << "resizing view " << viewNum << " to " << w << "x" << h << std::endl;

      w = std::max(1,w);
      h = std::max(1,h);

      vd.rgba.resize(w*h*4);
      vd.depth.resize(w*h);
   }
}

void RhrServer::deferredResize() {

    assert(!m_resizeBlocked);
    for (size_t i=0; i<numViews(); ++i) {
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
       for (size_t i=0; i<numViews(); ++i) {
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

unsigned RhrServer::timestep() const {

   return m_imageParam.timestep;
}

void RhrServer::setNumTimesteps(unsigned num) {

   if (num != m_numTimesteps) {
      m_numTimesteps = num;

      if (m_clientSocket) {
          animationMsg anim;
          anim.current = m_imageParam.timestep;
          anim.total = num;
          send(anim);
      }
   }
}

size_t RhrServer::updateCount() const {

    return m_updateCount;
}

const RhrServer::VariantVisibilityMap &RhrServer::getVariants() const {

    return m_clientVariants;
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

   send(msg);
}


//! handle request for a bounding sphere update
bool RhrServer::handleBounds(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const boundsMsg &bound) {

   if (bound.sendreply) {
      //std::cout << "SENDING BOUNDS" << std::endl;
      sendBoundsMessage(sock);
   }

   return true;
}

bool RhrServer::handleAnimation(std::shared_ptr<RhrServer::socket> sock, const vistle::message::RemoteRenderMessage &msg, const animationMsg &anim) {

    //CERR << "app timestep: " << anim.current << std::endl;
    m_imageParam.timestep = anim.current;
    return true;
}

bool RhrServer::handleVariant(std::shared_ptr<RhrServer::socket> sock, const vistle::message::RemoteRenderMessage &msg, const variantMsg &variant) {
    CERR << "app variant: " << variant.name << ", visible: " << variant.visible << std::endl;
    std::string name(variant.name);
    bool visible = variant.visible;
    m_clientVariants[name] = visible;
    ++m_updateCount;
    return true;
}

//! this is called before every frame, used for polling for RFB messages
void
RhrServer::preFrame() {

   if (m_delay) {
      usleep(m_delay);
   }

   m_io.poll();
   if (m_clientSocket) {
      bool received = false;
      do {
          received = false;
          size_t avail = 0;
          asio::socket_base::bytes_readable command(true);
          m_clientSocket->io_control(command);
          if (!m_clientSocket->is_open()) {
              resetClient();
              break;
          }
          if (command.get() > 0) {
              avail = command.get();
          }
          if (avail >= sizeof(message::RemoteRenderMessage)) {
              message::Buffer msg;
              std::vector<char> payload;
              if (message::recv(*m_clientSocket, msg, received, false, &payload) && received) {
                  switch(msg.type()) {
                  case message::REMOTERENDERING: {
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
                      case rfbVariant: {
                          auto &var = static_cast<const variantMsg &>(rhr);
                          handleVariant(m_clientSocket, m, var);
                          break;
                      }
                      case rfbTile:
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
          }
      } while (received);
   }
}

void RhrServer::invalidate(int viewNum, int x, int y, int w, int h, const RhrServer::ViewParameters &param, bool lastView) {

   if (m_clientSocket)
      encodeAndSend(viewNum, x, y, w, h, param, lastView);
}

namespace {

tileMsg *newTileMsg(const RhrServer::ImageParameters &param, const RhrServer::ViewParameters &vp, int viewNum, int x, int y, int w, int h) {

   assert(x+w <= std::max(0,vp.width));
   assert(y+h <= std::max(0,vp.height));

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
   message->unzippedsize = 0;

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
    const RhrServer::ImageParameters &param;

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
    , param(param)
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

#ifdef HAVE_ZFP
        if (param.depthZfp) {
            message->format = rfbDepthFloat;
            message->compression |= rfbTileDepthZfp;
        } else
#endif
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
    , param(param)
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
            if (msg.compression & rfbTileDepthZfp) {
#ifdef HAVE_ZFP
                zfp_type type = zfp_type_float;
                zfp_field *field = zfp_field_2d(const_cast<float *>(depth+y*stride+x), type, w, h);
                zfp_field_set_stride_2d(field, 0, stride);

                zfp_stream *zfp = zfp_stream_open(nullptr);
                switch (param.depthZfpMode) {
                default:
                    CERR << "invalid ZfpMode " << param.depthZfpMode << std::endl;
                    BOOST_FALLTHROUGH;
                case RhrServer::ZfpFixedRate:
                    zfp_stream_set_rate(zfp, 8, type, 2, 0);
                    break;
                case RhrServer::ZfpPrecision:
                    zfp_stream_set_precision(zfp, 16, type);
                    break;
                case RhrServer::ZfpAccuracy:
                    zfp_stream_set_accuracy(zfp, -4, type);
                    break;
                }
                size_t bufsize = zfp_stream_maximum_size(zfp, field);
                std::vector<char> zfpbuf(bufsize);
                bitstream *stream = stream_open(zfpbuf.data(), bufsize);
                zfp_stream_set_bit_stream(zfp, stream);

                zfp_stream_rewind(zfp);
                zfp_write_header(zfp, field, ZFP_HEADER_FULL);
                size_t zfpsize = zfp_compress(zfp, field);
                if (zfpsize == 0) {
                    CERR << "zfp compression failed" << std::endl;
                    msg.compression &= ~rfbTileDepthZfp;
                } else {
                    zfpbuf.resize(zfpsize);
                    result.payload = std::move(zfpbuf);
                    msg.size = zfpsize;
                    msg.unzippedsize = zfpsize;
                    msg.compression &= ~rfbTileDepthQuantize;
                }
                zfp_field_free(field);
                zfp_stream_close(zfp);
                stream_close(stream);
#endif
            } else if (msg.compression & rfbTileDepthQuantize) {
                const int ds = msg.format == rfbDepth16Bit ? 2 : 3;
                msg.size = depthquant_size(DepthFloat, ds, w, h);
                msg.unzippedsize = msg.size;
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
                msg.unzippedsize = msg.size;
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
                {
                   auto col = rgba + (msg.totalwidth*msg.y+msg.x)*bpp;
#ifdef TIMING
                   double start = vistle::Clock::time();
#endif
                   ret = tjCompress(tj.handle, col, msg.width, msg.totalwidth*bpp, msg.height, bpp, reinterpret_cast<unsigned char *>(jpegbuf.data()), &sz, subsamp, 90, TJ_BGR);
                   jpegbuf.resize(sz);
#ifdef TIMING
                   double dur = vistle::Clock::time() - start;
                   std::cerr << "JPEG compression: " << dur << "s, " << msg.width*(msg.height/dur)/1e6 << " MPix/s" << std::endl;
#endif
                }
                if (ret >= 0) {
                    msg.size = sz;
                    msg.unzippedsize = sz;
                    result.payload = std::move(jpegbuf);
                }
#endif
                if (ret < 0)
                    msg.compression &= ~rfbTileJpeg;
            }
            if (!(msg.compression & rfbTileJpeg)) {
                msg.unzippedsize = msg.size;
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
            sbuf.resize(compressed);
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

    finishTiles(param, lastView);
}

bool RhrServer::finishTiles(const RhrServer::ViewParameters &param, bool finish, bool sendTiles) {

    static int framecount=0;
    ++framecount;

    bool tileReady = false;
    do {
        RhrServer::EncodeResult result;
        tileReady = false;
        tileMsg *msg = nullptr;
        if (m_queuedTiles == 0 && finish) {
           msg = newTileMsg(m_imageParam, param, -1, 0, 0, 0, 0);
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
           if (m_queuedTiles == 0 && finish) {
              msg->flags |= rfbTileLast;
              //std::cerr << "last tile: req=" << msg.requestNumber << std::endl;
           }
           msg->frameNumber = framecount;
           if (sendTiles)
               send(*msg, &result.payload);
        }
        result.payload.clear();
        delete msg;
    } while (m_queuedTiles > 0 && (tileReady || finish));

    if (finish) {
        vassert(m_queuedTiles == 0);
        m_resizeBlocked = false;
        deferredResize();
    }

    return  m_queuedTiles==0;
}

RhrServer::ViewParameters RhrServer::getViewParameters(int viewNum) const {

    return m_viewData[viewNum].param;
}

} // namespace vistle
