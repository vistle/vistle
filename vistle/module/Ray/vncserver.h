/**\file
 * \brief VncServer plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013 HLRS
 *
 * \copyright GPL2+
 */

#ifndef VNC_SERVER_H
#define VNC_SERVER_H

#include <vector>
#include <deque>
#include <string>

#include <rfb/rfb.h>
#ifdef max
#undef max
#endif

#include <core/vector.h>

//! Implement remote hybrid rendering server based on VNC protocol
class VncServer
{
public:

   // plugin methods
   VncServer(int w, int h, unsigned short port=5900);
   ~VncServer();

   unsigned short port() const;

   int width() const;
   int height() const;
   unsigned char *rgba() const;
   float *depth();
   const float *depth() const;

   void resize(int w, int h);

   bool init(int w, int h, unsigned short port);
   void preFrame();
   void postFrame();
   void invalidate(int x, int y, int w, int h);

   void enableQuantization(bool value);
   void enableSnappy(bool value);
   void setDepthPrecision(int bits);

   int timestep() const;
   void setNumTimesteps(int num);

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

   const vistle::Matrix4 &viewMat() const;
   const vistle::Matrix4 &projMat() const;
   const vistle::Matrix4 &scaleMat() const;
   const vistle::Matrix4 &transformMat() const;
   const vistle::Matrix4 &viewerMat() const;

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
       uint32_t frameNumber;
       int timestep;
       double matrixTime;
       int width, height;
       vistle::Matrix4 proj;
       vistle::Matrix4 viewer;
       vistle::Matrix4 view;
       vistle::Matrix4 transform;
       vistle::Matrix4 scale;
       bool depthFloat; //!< whether depth should be retrieved as floating point
       int depthPrecision; //!< depth buffer read-back precision (bits) for integer formats
       bool depthQuant; //!< whether depth should be sent quantized
       bool depthSnappy; //!< whether depth should be entropy-encoded with SNAPPY
       bool rgbaJpeg;
       bool rgbaSnappy;

       ImageParameters()
       : frameNumber(0)
       , timestep(-1)
       , matrixTime(0.)
       , width(0)
       , height(0)
       , depthFloat(false)
       , depthPrecision(24)
       , depthQuant(false)
       , depthSnappy(false)
       , rgbaJpeg(false)
       , rgbaSnappy(false)
       {
           view = vistle::Matrix4::Identity();
           proj = vistle::Matrix4::Identity();
           transform = vistle::Matrix4::Identity();
           scale = vistle::Matrix4::Identity();
           viewer = vistle::Matrix4::Identity();
       }
   };

private:
   static VncServer *plugin; //<! access to plug-in from static member functions

   //! address of client to which a connection should be established (reverse connection)
   struct Client
   {
      std::string host; //!< host name
      int port; //!< TCP port number
   };
   std::vector<Client> m_clientList; //!< list of clients to which reverse connections should be tried
   
   bool m_benchmark; //!< whether timing information should be printed
   bool m_errormetric; //!< whether compression errors should be checked
   bool m_compressionrate; //!< whether compression ratio should be evaluated
#if 0
#endif
   double m_lastMatrixTime; //!< time when last matrix message was sent by client
   int m_delay; //!< artificial delay (us)
   ImageParameters m_param; //!< parameters for color/depth tiles
   bool m_resizeBlocked, m_resizeDeferred;
   int m_newWidth, m_newHeight;

   Screen m_screenConfig; //!< configuration for physical screen

   rfbScreenInfoPtr m_screen; //!< RFB protocol handler
   std::vector<float> m_depth;
   int m_numClients, m_numRhrClients;

#if 0
   vistle::Matrix4 m_transformMat;
   vistle::Matrix4 m_viewerMat;
   vistle::Matrix4 m_scaleMat;
   vistle::Matrix4 m_viewMat;
   vistle::Matrix4 m_projMat;
#endif

   vistle::Vector3 m_boundCenter;
   vistle::Scalar m_boundRadius;

   int m_numTimesteps;

   static void keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl);
   static void pointerEvent(int buttonmask, int x, int y, rfbClientPtr cl);

   static enum rfbNewClientAction newClientHook(rfbClientPtr cl);
   static void clientGoneHook(rfbClientPtr cl);
   static void sendBoundsMessage(rfbClientPtr cl);
   static void sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data);
   void broadcastApplicationMessage(int type, int length, const char *data);

   void encodeAndSend(int x, int y, int w, int h);
};
#endif
