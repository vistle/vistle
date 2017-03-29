/**\file
 * \brief RhrClient plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013, 2014 HLRS
 *
 * \copyright GPL2+
 */

#ifndef RHR_CLIENT_H
#define RHR_CLIENT_H

#include <string>
#include <mutex>
#include <thread>

#include <cover/coVRPluginSupport.h>

#include <rhr/rfbext.h>

#include <tbb/concurrent_queue.h>
#include <memory>

#include <osg/Geometry>
#include <osg/MatrixTransform>

//#define VRUI

#ifdef VRUI
#include <OpenVRUI/coMenuItem.h> // vrui::coMenuListener
#else
#include <cover/mui/support/EventListener.h>
#include <cover/coTabletUI.h>
#endif

#include <VistlePluginUtil/MultiChannelDrawer.h>

#include <boost/asio.hpp>

namespace asio = boost::asio;

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
using namespace vistle;

struct DecodeTask;
class RemoteConnection;

struct TileMessage {
    TileMessage(std::shared_ptr<tileMsg> msg, std::shared_ptr<std::vector<char>> payload)
        : msg(msg), payload(payload) {}

    std::shared_ptr<tileMsg> msg;
    std::shared_ptr<std::vector<char>> payload;
};

//! implement remote hybrid rendering client based on VNC protocol
class RhrClient: public coVRPlugin
#ifdef VRUI
, private coMenuListener
#else
, private mui::EventListener
, private opencover::coTUIListener
#endif
{
    friend class RemoteConnection;

private:

	unsigned GetCountChannels() const;
	void SetNumViewsAndChannelBase();

public:
   RhrClient();
   ~RhrClient();

   bool init() override;
   void addObject(const RenderObject *container, osg::Group *parent,
                  const RenderObject *geometry, const RenderObject *normals,
                  const RenderObject *colors, const RenderObject *texture) override;
   void removeObject(const char *objName, bool replaceFlag) override;
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

   void message(int type, int len, const void *msg) override;

   int handleRfbMessages();

private:
   //! make plugin available to static member functions
   static RhrClient *plugin;
   std::mutex m_pluginMutex;

   int m_compress; //!< VNC compression level (0: lowest, 9: highest)
   int m_quality; //!< VNC quality (0: lowest, 9: highest)
   //! do timings
   bool m_benchmark;
   double m_minDelay, m_maxDelay, m_accumDelay;
   double m_lastStat;
   double m_avgDelay;
   size_t m_remoteFrames, m_localFrames;
   size_t m_depthBytes, m_rgbBytes, m_depthBpp, m_numPixels;
   size_t m_depthBytesS, m_rgbBytesS, m_depthBppS, m_numPixelsS;

   bool connectClient(const std::string &connectionName, const std::string &address, unsigned short port);
   void clientCleanup(std::shared_ptr<RemoteConnection> &remote);
   bool sendMatricesMessage(std::shared_ptr<RemoteConnection> remote, std::vector<matricesMsg> &messages, uint32_t requestNum);
   bool sendLightsMessage(std::shared_ptr<RemoteConnection> remote);
   void fillMatricesMessage(matricesMsg &msg, int channel, int view, bool second=false);
   std::vector<matricesMsg> m_oldMatrices;

   //! server connection
   asio::io_service m_io;
   //std::shared_ptr<asio::ip::tcp::socket> m_sock;
   bool m_haveConnection;
   std::shared_ptr<RemoteConnection> m_remote;

   int m_requestedTimestep, m_remoteTimestep, m_visibleTimestep, m_numRemoteTimesteps, m_timestepToCommit;

   bool handleTileMessage(std::shared_ptr<tileMsg> msg, std::shared_ptr<std::vector<char>> payload);
   // work queue management for decoding tiles
   bool m_waitForDecode;
   int m_queued;
   std::deque<DecodeTask *> m_deferred;
   typedef tbb::concurrent_queue<std::shared_ptr<tileMsg> > ResultQueue;
   ResultQueue m_resultQueue;
   bool updateTileQueue();
   void handleTileMeta(const tileMsg &msg);
   void finishFrame(const tileMsg &msg);
   void swapFrame();
   bool checkSwapFrame();
   bool canEnqueue() const;
   void enqueueTask(DecodeTask *task);
   osg::ref_ptr<osg::Image> m_fbImg;
   int m_deferredFrames;

   bool m_frameReady;

   uint32_t m_matrixNum;

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
   mui::ToggleButton *m_reprojCheck, *m_adaptCheck, *m_adaptWithNeighborsCheck, *m_asMeshCheck, *m_withHolesCheck, *m_CubemapReprojectionCheck;
   mui::ToggleButton *m_inhibitModelUpdate;
#endif
   bool m_reproject, m_adapt, m_adaptWithNeighbors, m_asMesh, m_withHoles, m_ReprojectAsCubemap;
   bool m_noModelUpdate;
   osg::Matrix m_oldModelMatrix;

   osg::ref_ptr<opencover::MultiChannelDrawer> m_drawer;
   std::map<std::string, std::shared_ptr<RemoteConnection>> m_remotes;
   std::map<std::string, bool> m_coverVariants; //< whether a Variant is visible
};
#endif
