/**\file
 * \brief VncServer plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014 HLRS
 *
 * \copyright LGPL2+
 */

#ifndef VNC_SERVER_H
#define VNC_SERVER_H

#include <vector>
#include <deque>
#include <string>

#include <rfb/rfb.h>
#include <rhr/rfbext.h>
#ifdef max
#undef max
#endif

#include <core/vector.h>

#include <tbb/concurrent_queue.h>

#include "export.h"

//! Implement remote hybrid rendering server based on VNC protocol
class V_RHREXPORT VncServer
{
public:

  enum ColorCodec {
      Raw,
      Jpeg,
      Snappy,
  };

   // plugin methods
   VncServer(int w, int h, unsigned short port=5900);
   VncServer(int w, int h, const std::string &host, unsigned short port=5900);
   ~VncServer();

   unsigned short port() const;
   std::string host() const;

   int width(int viewNum) const;
   int height(int viewNum) const;
   unsigned char *rgba(int viewNum);
   const unsigned char *rgba(int viewNum) const;
   float *depth(int viewNum);
   const float *depth(int viewNum) const;

   void resize(int viewNum, int w, int h);

   int numClients() const;
   int numRhrClients() const;

   bool init(int w, int h, unsigned short port);
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

   void key(int type, int keySym, int mod);

   // other methods
   static rfbBool enableMatrices(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleMatricesMessage(rfbClientPtr cl, void *data,
         const rfbClientToServerMsg *message);

   static rfbBool enableLights(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleLightsMessage(rfbClientPtr cl, void *data,
         const rfbClientToServerMsg *message);

   static rfbBool enableBounds(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleBoundsMessage(rfbClientPtr cl, void *data,
         const rfbClientToServerMsg *message);

   static rfbBool enableTile(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleTileMessage(rfbClientPtr cl, void *data,
         const rfbClientToServerMsg *message);

   static rfbBool enableApplication(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleApplicationMessage(rfbClientPtr cl, void *data,
         const rfbClientToServerMsg *message);

   int numViews() const;
   const vistle::Matrix4 &viewMat(int viewNum) const;
   const vistle::Matrix4 &projMat(int viewNum) const;
   const vistle::Matrix4 &scaleMat(int viewNum) const;
   const vistle::Matrix4 &transformMat(int viewNum) const;
   const vistle::Matrix4 &viewerMat(int viewNum) const;

   void setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius);

   struct Screen {
      vistle::Vector3 pos;
      vistle::Vector3 hpr;
      vistle::Scalar hsize, vsize;
   };

   const Screen &screen() const;

   struct Light {
      vistle::Vector4 position;
      vistle::Vector3 direction;

      vistle::Vector3 transformedPosition;
      vistle::Vector3 transformedDirection;

      vistle::Vector4 ambient;
      vistle::Vector4 diffuse;
      vistle::Vector4 specular;

      vistle::Vector3 attenuation;
      vistle::Scalar spotCutoff;
      vistle::Scalar spotExponent;

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         ar & position;
         ar & direction;

         ar & transformedPosition;
         ar & transformedDirection;

         ar & ambient;
         ar & diffuse;
         ar & specular;

         ar & attenuation;
         ar & spotCutoff;
         ar & spotExponent;
      }
   };

   std::vector<Light> lights;

   struct ImageParameters {
       unsigned timestep;
       bool depthFloat; //!< whether depth should be retrieved as floating point
       int depthPrecision; //!< depth buffer read-back precision (bits) for integer formats
       bool depthQuant; //!< whether depth should be sent quantized
       bool depthSnappy; //!< whether depth should be entropy-encoded with SNAPPY
       bool rgbaJpeg;
       bool rgbaSnappy;

       ImageParameters()
       : timestep(0)
       , depthFloat(false)
       , depthPrecision(24)
       , depthQuant(false)
       , depthSnappy(false)
       , rgbaJpeg(false)
       , rgbaSnappy(false)
       {
       }
   };

   struct ViewParameters {
       uint32_t frameNumber;
       uint32_t requestNumber;
       double matrixTime;
       int width, height;
       vistle::Matrix4 proj;
       vistle::Matrix4 viewer;
       vistle::Matrix4 view;
       vistle::Matrix4 transform;
       vistle::Matrix4 scale;

       ViewParameters()
       : frameNumber(0)
       , requestNumber(0)
       , matrixTime(0.)
       , width(1)
       , height(1)
       {
           view = vistle::Matrix4::Identity();
           proj = vistle::Matrix4::Identity();
           transform = vistle::Matrix4::Identity();
           scale = vistle::Matrix4::Identity();
           viewer = vistle::Matrix4::Identity();
       }

      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {

         std::cout << "SER ViewParameters" << std::endl << std::flush;
         ar & frameNumber;
         ar & requestNumber;
         ar & matrixTime;
         ar & width;
         ar & height;

         ar & proj;
         ar & viewer;
         ar & view;
         ar & transform;
         ar & scale;
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

   void broadcastApplicationMessage(int type, int length, const char *data);
private:
   static VncServer *plugin; //<! access to plug-in from static member functions
   AppMessageHandlerFunc m_appHandler;

   int m_tileWidth, m_tileHeight;

   //! address of client to which a connection should be established (reverse connection)
   struct Client
   {
      std::string host; //!< host name
      unsigned short port; //!< TCP port number

      Client(): port(0) {}
      Client(const std::string &host, unsigned short port): host(host), port(port) {}
   };
   std::vector<Client> m_clientList; //!< list of clients to which reverse connections should be tried
   std::vector<ViewData> m_viewData;
   
   bool m_benchmark; //!< whether timing information should be printed
   bool m_errormetric; //!< whether compression errors should be checked
   bool m_compressionrate; //!< whether compression ratio should be evaluated
   double m_lastMatrixTime; //!< time when last matrix message was sent by client
   int m_delay; //!< artificial delay (us)
   ImageParameters m_imageParam; //!< parameters for color/depth codec
   bool m_resizeBlocked, m_resizeDeferred;

   Screen m_screenConfig; //!< configuration for physical screen

   rfbScreenInfoPtr m_screen; //!< RFB protocol handler
   int m_numClients, m_numRhrClients;

   vistle::Vector3 m_boundCenter;
   vistle::Scalar m_boundRadius;

   unsigned m_numTimesteps;

   static void keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl);
   static void pointerEvent(int buttonmask, int x, int y, rfbClientPtr cl);

   static enum rfbNewClientAction newClientHook(rfbClientPtr cl);
   static void clientGoneHook(rfbClientPtr cl);
   static void sendBoundsMessage(rfbClientPtr cl);
   static void sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data);

   void encodeAndSend(int viewNum, int x, int y, int w, int h, const ViewParameters &param, bool lastView);

   struct EncodeResult {

       EncodeResult(tileMsg *msg=nullptr)
           : message(msg)
           , payload(nullptr)
           {}

       tileMsg *message;
       const char *payload;
   };

   friend struct EncodeTask;

   tbb::concurrent_queue<EncodeResult> m_resultQueue;
   size_t m_queuedTiles;
   bool m_firstTile;

   void deferredResize();
};
#endif
