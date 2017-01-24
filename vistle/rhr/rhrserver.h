/**\file
 * \brief RhrServer plugin class
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

#include <util/enum.h>
#include <core/vector.h>
#include <renderer/renderobject.h>

#include <tbb/concurrent_queue.h>

#include "export.h"

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

   DEFINE_ENUM_WITH_STRING_CONVERSIONS(ColorCodec,
                                     (Raw)
                                     (Jpeg_YUV411)
                                     (Jpeg_YUV444)
                                     (Snappy)
                                     )

   RhrServer(unsigned short port=31313);
   ~RhrServer();

   unsigned short port() const;
   address listenAddress() const;
   asio::io_service &ioService();

   int width(int viewNum) const;
   int height(int viewNum) const;
   unsigned char *rgba(int viewNum);
   const unsigned char *rgba(int viewNum) const;
   float *depth(int viewNum);
   const float *depth(int viewNum) const;

   void resize(int viewNum, int w, int h);

   bool init(unsigned short port);
   bool start(unsigned short port);
   void preFrame();

   typedef bool (*AppMessageHandlerFunc)(int type, const std::vector<char> &msg);
   void setAppMessageHandler(AppMessageHandlerFunc handler);

   struct ViewParameters;
   ViewParameters getViewParameters(int viewNum) const;
   void invalidate(int viewNum, int x, int y, int w, int h, const ViewParameters &param, bool lastView);

   void setColorCodec(ColorCodec value);
   void enableQuantization(bool value);
   void enableDepthSnappy(bool value);
   void setDepthPrecision(int bits);
   void setTileSize(int w, int h);

   unsigned timestep() const;
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

   int numViews() const;
   const vistle::Matrix4 &viewMat(int viewNum) const;
   const vistle::Matrix4 &projMat(int viewNum) const;
   const vistle::Matrix4 &modelMat(int viewNum) const;

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
       unsigned timestep;
       bool depthFloat; //!< whether depth should be retrieved as floating point
       int depthPrecision; //!< depth buffer read-back precision (bits) for integer formats
       bool depthQuant; //!< whether depth should be sent quantized
       bool depthSnappy; //!< whether depth should be entropy-encoded with SNAPPY
       bool rgbaJpeg;
       bool rgbaSnappy;
       bool rgbaChromaSubsamp;

       ImageParameters()
       : timestep(0)
       , depthFloat(false)
       , depthPrecision(24)
       , depthQuant(false)
       , depthSnappy(false)
       , rgbaJpeg(false)
       , rgbaSnappy(false)
       , rgbaChromaSubsamp(false)
       {
       }
   };

   struct ViewParameters {
       vistle::Matrix4 proj;
       vistle::Matrix4 view;
       vistle::Matrix4 model;
       uint32_t frameNumber;
       uint32_t requestNumber;
       int32_t timestep;
       double matrixTime;
       int width, height;

       ViewParameters()
       : frameNumber(0)
       , requestNumber(0)
       , timestep(0)
       , matrixTime(0.)
       , width(1)
       , height(1)
       {
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
   static RhrServer *plugin; //<! access to plug-in from static member functions
   asio::io_service m_io;
   asio::ip::tcp::acceptor m_acceptor;
   std::shared_ptr<asio::ip::tcp::socket> m_clientSocket;
   unsigned short m_port;
   asio::ip::address m_listenAddress;
   AppMessageHandlerFunc m_appHandler;

   bool startAccept();
   void handleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> sock, const boost::system::error_code &error);

   size_t m_updateCount = 0;

   int m_tileWidth, m_tileHeight;

   std::vector<ViewData, Eigen::aligned_allocator<ViewData>> m_viewData;
   
   bool m_benchmark; //!< whether timing information should be printed
   bool m_errormetric; //!< whether compression errors should be checked
   bool m_compressionrate; //!< whether compression ratio should be evaluated
   double m_lastMatrixTime; //!< time when last matrix message was sent by client
   int m_delay; //!< artificial delay (us)
   ImageParameters m_imageParam; //!< parameters for color/depth codec
   bool m_resizeBlocked, m_resizeDeferred;

   vistle::Vector3 m_boundCenter;
   vistle::Scalar m_boundRadius;

   unsigned m_numTimesteps;

   void sendBoundsMessage(std::shared_ptr<socket> sock);

   void encodeAndSend(int viewNum, int x, int y, int w, int h, const ViewParameters &param, bool lastView);

   struct EncodeResult {

       EncodeResult(tileMsg *msg=nullptr)
           : message(msg)
           {}

       tileMsg *message;
       std::vector<char> payload;
   };

   friend struct EncodeTask;

   tbb::concurrent_queue<EncodeResult> m_resultQueue;
   size_t m_queuedTiles;
   bool m_firstTile;

   void deferredResize();

   VariantVisibilityMap m_clientVariants;
   InitialVariantVisibilityMap m_localVariants;
};

} // namespace vistle
#endif
