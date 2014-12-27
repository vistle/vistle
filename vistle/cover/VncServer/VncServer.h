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

#include <kernel/coVRPluginSupport.h>

#include <deque>
#include <string>

#include <rfb/rfb.h>

using namespace covise;
using namespace opencover;

class ReadBackCuda;

//! Implement remote hybrid rendering server based on VNC protocol
class VncServer: public coVRPlugin
{
public:
   // plugin methods
   VncServer();
   ~VncServer();
   bool init();
   void preFrame();
   void postSwapBuffers(int windowNumber);
   void addObject(RenderObject *baseObj,
         RenderObject *geomObj, RenderObject *normObj,
         RenderObject *colorObj, RenderObject *texObj,
         osg::Group *parent,
         int numCol, int colorBinding, int colorPacking,
         float *r, float *g, float *b, int *packedCol,
         int numNormals, int normalBinding,
         float *xn, float *yn, float *zn,
         float transparency);
   void removeObject(const char *objName, bool replaceFlag);

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

private:
   static VncServer *plugin; //<! access to plug-in from static member functions

   //! address of client to which a connection should be established (reverse connection)
   struct Client
   {
      std::string host; //!< host name
      int port; //!< TCP port number
   };
   std::vector<Client> m_clientList; //!< list of clients to which reverse connections should be tried
   
   ReadBackCuda *m_cudaColor; //!< color transfer using CUDA, if available
   ReadBackCuda *m_cudaDepth; //!< depth transfer using CUDA, if available
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

   void readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
	GLenum format, int ps, GLubyte *bits, GLint buf, GLenum type=GL_UNSIGNED_BYTE);
   bool m_cudapinnedmemory; //!< wether CUDA host memory should be pinned

   rfbScreenInfoPtr m_screen; //!< RFB protocol handler
   int m_width, m_height; //! size of framebuffer
   int m_numRhrClients;

   static void keyEvent(rfbBool down, rfbKeySym sym, rfbClientPtr cl);
   static void pointerEvent(int buttonmask, int x, int y, rfbClientPtr cl);

   static void clientGoneHook(rfbClientPtr cl);
   static void sendDepthMessage(rfbClientPtr cl);
   static void sendApplicationMessage(rfbClientPtr cl, int type, int length, const char *data);
   static void broadcastApplicationMessage(int type, int length, const char *data);
   static void broadcastAddObject(RenderObject *base,
         bool isBase=false,
         RenderObject *geo=NULL,
         RenderObject *norm=NULL,
         RenderObject *col=NULL,
         RenderObject *tex=NULL);
};
#endif
