/**\file
 * \brief RhrServer class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014 HLRS
 *
 * \copyright LGPL2+
 */

#ifndef RHR_SERVER_H
#define RHR_SERVER_H

#include <vector>
#include <deque>
#include <string>
#include <map>

#include <boost/asio.hpp>

#include <rhr/rfbext.h>

#include <core/vector.h>
#include <renderer/renderobject.h>

#include <tbb/concurrent_queue.h>

#include "export.h"
#include "compdecomp.h"

namespace vistle {

namespace asio = boost::asio;
using message::RemoteRenderMessage;

//! Implement remote hybrid rendering server
class V_RHREXPORT RhrServer
{
public:
   typedef asio::ip::tcp::socket socket;
   typedef asio::ip::tcp::acceptor acceptor;
   typedef asio::ip::address address;

   RhrServer();
   ~RhrServer();

   int numClients() const;

   bool isConnecting() const;

   unsigned short port() const;
   address listenAddress() const;

   unsigned short destinationPort() const;
   const std::string &destinationHost() const;

   asio::io_service &ioService();

   int width(size_t viewNum) const;
   int height(size_t viewNum) const;
   unsigned char *rgba(size_t viewNum);
   const unsigned char *rgba(size_t viewNum) const;
   float *depth(size_t viewNum);
   const float *depth(size_t viewNum) const;

   void resize(size_t viewNum, int w, int h);

   void init();
   bool startServer(unsigned short port);
   bool makeConnection(const std::string &host, unsigned short port, int secondsToTry=-1);
   void preFrame();

   struct ViewParameters;
   ViewParameters getViewParameters(int viewNum) const;
   void invalidate(int viewNum, int x, int y, int w, int h, ViewParameters param, bool lastView);
   void updateModificationCount();

   void setColorCodec(CompressionParameters::ColorCodec value);
   void setColorCompression(message::CompressionMode mode);
   void enableDepthZfp(bool value);
   void enableQuantization(bool value);
   void setDepthPrecision(int bits);
   void setDepthCompression(message::CompressionMode mode);
   void setTileSize(int w, int h);
   void setZfpMode(DepthCompressionParameters::ZfpMode mode);
   void setDumpImages(bool enable);

   int timestep() const;
   void setNumTimesteps(unsigned num);
   size_t updateCount() const;
   typedef std::map<std::string, vistle::RenderObject::InitialVariantVisibility> InitialVariantVisibilityMap;
   typedef std::map<std::string, bool> VariantVisibilityMap;
   const VariantVisibilityMap &getVariants() const;

   bool handleMatrices(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const matricesMsg &mat);
   bool handleLights(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const lightsMsg &light);
   bool handleBounds(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const boundsMsg &bound);
   bool handleAnimation(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const animationMsg &anim);
   bool handleVariant(std::shared_ptr<socket> sock, const RemoteRenderMessage &msg, const variantMsg &variant);

   size_t numViews() const;
   const vistle::Matrix4 &viewMat(size_t viewNum) const;
   const vistle::Matrix4 &projMat(size_t viewNum) const;
   const vistle::Matrix4 &modelMat(size_t viewNum) const;

   void setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius);

   struct Screen {
      vistle::Vector3 pos;
      vistle::Scalar hsize;
      vistle::Vector3 hpr;
      vistle::Scalar vsize;
   };

   struct Light {
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW

      vistle::Vector4 position;

      vistle::Vector4 ambient;
      vistle::Vector4 diffuse;
      vistle::Vector4 specular;

      vistle::Vector3 attenuation;
      vistle::Scalar spotCutoff;
      vistle::Vector3 direction;
      vistle::Scalar spotExponent;

      mutable vistle::Vector4 transformedPosition;
      mutable vistle::Vector3 transformedDirection;
      bool enabled;
      mutable bool isDirectional;

      bool operator==(const Light &rhs) const {

          if (position != rhs.position)
              return false;
          if (attenuation != rhs.attenuation)
              return false;
          if (ambient != rhs.ambient)
              return false;
          if (diffuse != rhs.diffuse)
              return false;
          if (specular != rhs.specular)
              return false;
          if (direction != rhs.direction)
              return false;
          if (spotCutoff != rhs.spotCutoff)
              return false;
          if (spotExponent != rhs.spotExponent)
              return false;
          if (enabled != rhs.enabled)
              return false;

          return true;
      }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & enabled;

         ar & position;
         ar & attenuation;

         ar & ambient;
         ar & diffuse;
         ar & specular;

         ar & direction;
         ar & spotCutoff;
         ar & spotExponent;
      }
   };

   std::vector<Light> lights;
   size_t lightsUpdateCount;

   struct ImageParameters {
       int timestep = -1;
       double timestepRequestTime = -1.; // time for last animationMsg, for latency measurement
       DepthCompressionParameters depthParam;
#if 0
       bool depthFloat; //!< whether depth should be retrieved as floating point
       int depthPrecision; //!< depth buffer read-back precision (bits) for integer formats
       bool depthZfp; //!< whether depth should be compressed with floating point compressor zfp
       bool depthQuant; //!< whether depth should be sent quantized
       ZfpMode depthZfpMode;
       message::CompressionMode depthCompress;
#endif
       RgbaCompressionParameters rgbaParam;
#if 0
       bool rgbaJpeg;
       bool rgbaChromaSubsamp;
       message::CompressionMode rgbaCompress;
#endif

       ImageParameters()
       : timestep(-1)
#if 0
       , depthFloat(false)
       , depthPrecision(24)
       , depthZfp(false)
       , depthQuant(false)
       , depthZfpMode(ZfpAccuracy)
       , depthCompress(message::CompressionNone)
       , rgbaJpeg(false)
       , rgbaChromaSubsamp(false)
       , rgbaCompress(message::CompressionNone)
#endif
       {
       }
   };

   struct ViewParameters {
       vistle::Matrix4 head;
       vistle::Matrix4 proj;
       vistle::Matrix4 view;
       vistle::Matrix4 model;
       uint32_t frameNumber;
       uint32_t requestNumber;
       int32_t timestep = -1;
       double matrixTime;
       int width, height;
       uint8_t eye = 0;

       ViewParameters()
       : frameNumber(0)
       , requestNumber(0)
       , timestep(-1)
       , matrixTime(0.)
       , width(1)
       , height(1)
       {
           head = vistle::Matrix4::Identity();
           proj = vistle::Matrix4::Identity();
           view = vistle::Matrix4::Identity();
           model = vistle::Matrix4::Identity();
       }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & frameNumber;
         ar & requestNumber;
         ar & timestep;
         ar & matrixTime;
         ar & width;
         ar & height;
         ar & eye;

         ar & head;
         ar & proj;
         ar & view;
         ar & model;
      }
   };

   struct ViewData {

       ViewParameters param; //!< parameters for color/depth tiles
       ViewParameters nparam; //!< parameters for color/depth tiles currently being updated
       int newWidth, newHeight; //!< in case resizing was blocked while message was received
       std::vector<unsigned char> rgba;
       std::vector<float> depth;

       ViewData(): newWidth(-1), newHeight(-1) {}
   };
   void updateVariants(const std::vector<std::pair<std::string, vistle::RenderObject::InitialVariantVisibility> > &added, const std::vector<std::string> &removed);

private:
   asio::io_service m_io;
   asio::ip::tcp::acceptor m_acceptor;
   bool m_listen;
   std::shared_ptr<asio::ip::tcp::socket> m_clientSocket;
   unsigned short m_port;
   asio::ip::address m_listenAddress;
   unsigned short m_destPort;
   std::string m_destHost;

   bool startAccept();
   void handleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);

   size_t m_updateCount = 0;

   int m_tileWidth, m_tileHeight;

   std::vector<ViewData, Eigen::aligned_allocator<ViewData>> m_viewData;

   int m_delay; //!< artificial delay (us)
   ImageParameters m_imageParam; //!< parameters for color/depth codec
   bool m_resizeBlocked, m_resizeDeferred;

   vistle::Vector3 m_boundCenter;
   vistle::Scalar m_boundRadius;

   unsigned m_numTimesteps;

   void sendBoundsMessage(std::shared_ptr<socket> sock);

   void encodeAndSend(int viewNum, int x, int y, int w, int h, const ViewParameters &param, bool lastView);
   bool finishTiles(const ViewParameters &param, bool wait, bool sendTiles=true);

   struct EncodeResult {

       EncodeResult(tileMsg *msg=nullptr)
           : message(msg)
           {}

       tileMsg *message = nullptr;
       RemoteRenderMessage *rhrMessage = nullptr;
       std::vector<char> payload;
   };

   friend struct EncodeTask;

   tbb::concurrent_queue<EncodeResult> m_resultQueue;
   size_t m_queuedTiles;
   bool m_firstTile;

   void deferredResize();

   VariantVisibilityMap m_clientVariants;
   InitialVariantVisibilityMap m_localVariants;

   bool send(const RemoteRenderMessage &msg, const std::vector<char> *payload=nullptr);
   void resetClient();
   int m_framecount = 0;
   bool m_dumpImages = false;
   size_t m_modificationCount = 0;
};

} // namespace vistle
#endif
