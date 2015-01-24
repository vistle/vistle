/**\file
 * \brief VncClient plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013, 2014 HLRS
 *
 * \copyright GPL2+
 */

#ifndef VNC_CLIENT_H
#define VNC_CLIENT_H

#include <cover/coVRPluginSupport.h>

#include <string>

#include <rfb/rfb.h>
#include <rfb/rfbclient.h>

#include <rhr/rfbext.h>

#include <tbb/concurrent_queue.h>
#include <boost/shared_ptr.hpp>

#include <osg/Geometry>
#include <osg/MatrixTransform>

#include <OpenVRUI/coMenuItem.h> // vrui::coMenuListener

namespace osg {
class TextureRectangle;
}

namespace vrui {
class coSubMenuItem;
class coRowMenu;
class coCheckboxMenuItem;
class coPotiMenuItem;
}

using namespace vrui;
using namespace opencover;

class RemoteRenderObject;

struct DecodeTask;

//! implement remote hybrid rendering client based on VNC protocol
class VncClient: public coVRPlugin, private coMenuListener
{
public:
   VncClient();
   ~VncClient();

   bool init();
   void preFrame();
   void expandBoundingSphere(osg::BoundingSphere &bs);
   void menuEvent(coMenuItem* item);

   static rfbBool rfbMatricesMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbLightsMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbBoundsMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbDepthMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbTileMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbApplicationMessage(rfbClient *client, rfbServerToClientMsg *message);
   static void rfbUpdate(rfbClient *client, int x, int y, int w, int h);
   static rfbBool rfbResize(rfbClient *client);

   void sendFeedback(const char *info, const char *key, const char *data=NULL);

   //! store data associated with one screen
   struct ScreenData {
      int screenNum;
      bool second;
      osg::Matrix curProj, curView, curTransform, curScale;
      osg::Matrix newProj, newView, newTransform, newScale;

   // geometry for mapping depth image
      osg::TextureRectangle *colorTex;
      osg::TextureRectangle *depthTex;
      osg::Vec2Array* texcoord;
      osg::ref_ptr<osg::Geometry> fixedGeo;
      osg::ref_ptr<osg::Geometry> reprojGeo;
      osg::Uniform *size;
      osg::Uniform *reprojMat;
      osg::Geode *geode;
      osg::ref_ptr<osg::MatrixTransform> scene;
      osg::ref_ptr<osg::Camera> camera;

      ScreenData(int screen=-1)
         : screenNum(screen)
         , second(false)
         , colorTex(NULL)
         , depthTex(NULL)
         , texcoord(NULL)
         , size(NULL)
         , reprojMat(NULL)
         , geode(NULL)
      {
      }
   };

private:
   //! make plugin available to static member functions
   static VncClient *plugin;

   void switchReprojection(bool reproj);
   void initScreenData(VncClient::ScreenData &sd);
   void createGeometry(VncClient::ScreenData &sd);
   void setPointSize(float sz);

   osg::ref_ptr<osg::Camera> m_remoteCam;

   int m_compress; //!< VNC compression level (0: lowest, 9: highest)
   int m_quality; //!< VNC quality (0: lowest, 9: highest)
   //! do timings
   bool m_benchmark;
   double m_minDelay, m_maxDelay, m_accumDelay;
   double m_lastStat;
   size_t m_remoteFrames, m_localFrames;
   size_t m_depthBytes, m_rgbBytes, m_depthBpp, m_numPixels;
   size_t m_depthBytesS, m_rgbBytesS, m_depthBppS, m_numPixelsS;

   std::vector<ScreenData> m_screenData;

   int handleRfbMessages();
   bool connectClient();
   void clientCleanup(rfbClient *client);
   void sendMatricesMessage(rfbClient *client, std::vector<matricesMsg> &messages, uint32_t requestNum);
   void sendLightsMessage(rfbClient *client);
   void sendApplicationMessage(rfbClient *client, int type, int length, const void *data);
   void fillMatricesMessage(matricesMsg &msg, int screen, int view, bool second=false);

   //! server connection
   rfbClient *m_client;
   bool m_listen;
   bool m_haveConnection;

   appScreenConfig activeConfig;

   //! buffer for storing decompressed data
   std::vector<char> recvbuf, decompbuf;

   //! mapping from object names to RenderObjects
   typedef std::map<std::string, RemoteRenderObject *> ObjectMap;
   ObjectMap m_objectMap;
   RemoteRenderObject *findObject(const std::string &name) const;

   int m_timestep, m_totalTimesteps, m_remoteTimesteps;

   bool handleTileMessage(boost::shared_ptr<tileMsg> msg, boost::shared_ptr<char> payload);
   // work queue management for decoding tiles
   bool m_waitForDecode;
   int m_queued;
   std::deque<DecodeTask *> m_deferred;
   typedef tbb::concurrent_queue<boost::shared_ptr<tileMsg> > ResultQueue;
   ResultQueue m_resultQueue;
   bool updateTileQueue();
   void handleTileMeta(const tileMsg &msg);
   void finishFrame(const tileMsg &msg);
   void swapFrame();
   bool canEnqueue() const;
   void enqueueTask(DecodeTask *task);
   osg::ref_ptr<osg::Image> m_fbImg;
   int m_deferredFrames;

   bool m_frameReady;

   uint32_t m_matrixNum;

   struct TileMessage {
      TileMessage(boost::shared_ptr<tileMsg> msg, boost::shared_ptr<char> payload)
      : msg(msg), payload(payload) {}

      boost::shared_ptr<tileMsg> msg;
      boost::shared_ptr<char> payload;
   };
   std::deque<TileMessage> m_receivedTiles;
   int m_lastTileAt;

   int m_screenBase;
   int m_numViews;
   std::vector<int> m_numScreens;

   coRowMenu *m_menu;
   coSubMenuItem *m_menuItem;
   coCheckboxMenuItem *m_reprojCheck;
   coCheckboxMenuItem *m_allTilesCheck;
   coPotiMenuItem *m_pointSizePoti;
   bool m_reproject;
   float m_pointSize;
};
#endif
