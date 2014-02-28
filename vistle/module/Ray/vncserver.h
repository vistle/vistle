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
   float *depth() const;

   void resize(int w, int h);

   bool init(int w, int h, unsigned short port);
   void preFrame();
   void postFrame();
   void invalidate(int x, int y, int w, int h);

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

   static rfbBool enableDepth(rfbClientPtr cl, void **data, int encoding);
   static rfbBool handleDepthMessage(rfbClientPtr cl, void *data,
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
   int m_depthprecision; //!< depth buffer read-back precision (bits) for integer formats
   bool m_depthfloat; //!< whether depth should be retrieved as floating point
   bool m_depthquant; //!< whether depth should be sent quantized
   bool m_depthcopy; //!< whether depth should be copied to color for read-back
   bool m_glextChecked; //!< whether availability of GL extensions has been already checked
   double m_lastMatrixTime; //!< time when last matrix message was sent by client
   int m_delay; //!< artificial delay (us)

   Screen m_screenConfig; //!< configuration for physical screen

   rfbScreenInfoPtr m_screen; //!< RFB protocol handler
   float *m_depth;
   bool m_depthSent; //!< whether depth has already sent to clients
   int m_width, m_height; //! size of framebuffer
   int m_numRhrClients;

   vistle::Matrix4 m_transformMat;
   vistle::Matrix4 m_viewerMat;
   vistle::Matrix4 m_scaleMat;
   vistle::Matrix4 m_viewMat;
   vistle::Matrix4 m_projMat;

   vistle::Vector3 m_boundCenter;
   vistle::Scalar m_boundRadius;

   int m_timestep, m_numTimesteps;

   static void keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl);
   static void pointerEvent(int buttonmask, int x, int y, rfbClientPtr cl);

   static void clientGoneHook(rfbClientPtr cl);
   static void sendDepthMessage(rfbClientPtr cl);
   static void sendBoundsMessage(rfbClientPtr cl);
   static void sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data);
   void broadcastApplicationMessage(int type, int length, const char *data);
};
#endif
