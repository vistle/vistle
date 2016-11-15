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

//#define VRUI

#ifdef VRUI
#include <OpenVRUI/coMenuItem.h> // vrui::coMenuListener
#else
#include <cover/mui/support/EventListener.h>
#include <cover/coTabletUI.h>
#endif

#include <PluginUtil/MultiChannelDrawer.h>

namespace boost {
class thread;
class recursive_mutex;
}

namespace osg {
class TextureRectangle;
}

namespace vrui {
class coSubMenuItem;
class coRowMenu;
class coCheckboxMenuItem;
}

namespace mui {
class Tab;
class ToggleButton;
}

using namespace vrui;
using namespace opencover;

class RemoteRenderObject;

struct DecodeTask;


//! implement remote hybrid rendering client based on VNC protocol
class VncClient: public coVRPlugin
#ifdef VRUI
, private coMenuListener
#else
, private mui::EventListener
, private opencover::coTUIListener
#endif
{
public:
   VncClient();
   ~VncClient();

   bool init() override;
   void preFrame() override;
   void expandBoundingSphere(osg::BoundingSphere &bs) override;
#ifdef VRUI
   void menuEvent(coMenuItem* item) override;
#else
   void muiEvent(mui::Element *muiItem) override;
   void tabletEvent(coTUIElement *) override;
#endif
   void setTimestep(int t) override;
   void requestTimestep(int t) override;

   static rfbBool rfbMatricesMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbLightsMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbBoundsMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbDepthMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbTileMessage(rfbClient *client, rfbServerToClientMsg *message);
   static rfbBool rfbApplicationMessage(rfbClient *client, rfbServerToClientMsg *message);
   static void rfbUpdate(rfbClient *client, int x, int y, int w, int h);
   static rfbBool rfbResize(rfbClient *client);

   void sendFeedback(const char *info, const char *key, const char *data=NULL);
   int handleRfbMessages();

   bool m_runClient;
   boost::recursive_mutex *m_clientMutex;
private:
   //! make plugin available to static member functions
   static VncClient *plugin;

   std::string m_serverHost;
   int m_port;
   int m_compress; //!< VNC compression level (0: lowest, 9: highest)
   int m_quality; //!< VNC quality (0: lowest, 9: highest)
   //! do timings
   bool m_benchmark;
   double m_minDelay, m_maxDelay, m_accumDelay;
   double m_lastStat;
   size_t m_remoteFrames, m_localFrames;
   size_t m_depthBytes, m_rgbBytes, m_depthBpp, m_numPixels;
   size_t m_depthBytesS, m_rgbBytesS, m_depthBppS, m_numPixelsS;

   bool connectClient();
   void clientCleanup(rfbClient *client);
   void sendMatricesMessage(rfbClient *client, std::vector<matricesMsg> &messages, uint32_t requestNum);
   void sendLightsMessage(rfbClient *client);
   void sendApplicationMessage(rfbClient *client, int type, int length, const void *data);
   void fillMatricesMessage(matricesMsg &msg, int channel, int view, bool second=false);

   //! server connection
   rfbClient *m_client;
   boost::thread *m_clientThread;
   bool m_listen;
   bool m_haveConnection;
   bool m_haveMessage;

   appScreenConfig activeConfig;

   //! buffer for storing decompressed data
   std::vector<char> recvbuf, decompbuf;

   //! mapping from object names to RenderObjects
   typedef std::map<std::string, RemoteRenderObject *> ObjectMap;
   ObjectMap m_objectMap;
   RemoteRenderObject *findObject(const std::string &name) const;

   int m_requestedTimestep, m_remoteTimestep, m_visibleTimestep, m_numRemoteTimesteps, m_timestepToCommit;

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
   void checkSwapFrame();
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

   int m_channelBase;
   int m_numViews;
   std::vector<int> m_numChannels;

   MultiChannelDrawer::Mode m_mode;
   void modeToUi(MultiChannelDrawer::Mode mode);
   void applyMode();

#ifdef VRUI
   coRowMenu *m_menu;
   coSubMenuItem *m_menuItem;
   coCheckboxMenuItem *m_reprojCheck, *m_adaptCheck;
   coCheckboxMenuItem *m_allTilesCheck;
#else
   mui::Tab *m_tab;
   mui::ToggleButton *m_reprojCheck, *m_adaptCheck, *m_connectCheck, *m_adaptWithNeighborsCheck, *m_asMeshCheck, *m_withHolesCheck;
   coTUILabel *m_hostLabel, *m_portLabel;
   coTUIEditTextField *m_hostEdit;
   coTUIEditIntField *m_portEdit;
   mui::ToggleButton *m_inhibitModelUpdate;
#endif
   bool m_reproject, m_adapt, m_adaptWithNeighbors, m_asMesh, m_withHoles;
   bool m_noModelUpdate;
   osg::Matrix m_oldModelMatrix;

   osg::ref_ptr<opencover::MultiChannelDrawer> m_drawer;
};
#endif
