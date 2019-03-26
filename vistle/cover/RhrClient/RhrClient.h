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
#include <memory>

#include <cover/coVRPlugin.h>
#include <cover/ui/Owner.h>

#include <rhr/rfbext.h>

#include <boost/asio.hpp>

#include "TileMessage.h"
#include "RemoteConnection.h"
#include "NodeConfig.h"

namespace opencover {
namespace ui {
class Action;
class Button;
class Menu;
class SelectionList;
}
}

class VistleInteractor;

class RemoteConnection;

//! implement remote hybrid rendering client based on VNC protocol
class RhrClient: public opencover::coVRPlugin, public opencover::ui::Owner
{
    friend class RemoteConnection;
    using GeometryMode = RemoteConnection::GeometryMode;
    using ViewSelection = RemoteConnection::ViewSelection;

public:
   RhrClient();
   ~RhrClient();

   bool init() override;
   void addObject(const opencover::RenderObject *container, osg::Group *parent,
                  const opencover::RenderObject *geometry, const opencover::RenderObject *normals,
                  const opencover::RenderObject *colors, const opencover::RenderObject *texture) override;
   void removeObject(const char *objName, bool replaceFlag) override;
   void newInteractor(const opencover::RenderObject *container, opencover::coInteractor *it) override;
   bool update() override;
   void preFrame() override;
   void clusterSyncDraw() override;
   void expandBoundingSphere(osg::BoundingSphere &bs) override;
   void setTimestep(int t) override;
   void requestTimestep(int t) override;

   void message(int toWhom, int type, int len, const void *msg) override;

   bool updateViewer() override;

private:
   //! do timings
   bool m_benchmark = false;
   double m_lastStat = -1.;
   size_t m_localFrames = 0;
   int m_remoteSkipped = 0;


   std::shared_ptr<RemoteConnection> connectClient(const std::string &connectionName, const std::string &address, unsigned short port);
   std::shared_ptr<RemoteConnection> startListen(const std::string &connectionName, unsigned short port, unsigned short portLast=0);
   void addRemoteConnection(const std::string &name, std::shared_ptr<RemoteConnection> remote);
   typedef std::map<std::string, std::shared_ptr<RemoteConnection>> RemotesMap;
   RemotesMap::iterator removeRemoteConnection(RemotesMap::iterator it);
   bool sendMatricesMessage(std::shared_ptr<RemoteConnection> remote, std::vector<vistle::matricesMsg> &messages, uint32_t requestNum);
   vistle::lightsMsg buildLightsMessage();
   void fillMatricesMessage(vistle::matricesMsg &msg, int channel, int view, bool second=false);
   std::vector<vistle::matricesMsg> gatherAllMatrices();

   //! server connection
   boost::asio::io_service m_io;
   bool m_clientsChanged = false;

   int m_requestedTimestep, m_visibleTimestep, m_numRemoteTimesteps;

   // work queue management for decoding tiles
   bool finishFrames();
   bool swapFrames();
   bool checkAdvanceFrame();
   bool updateRemotes();
   bool syncRemotesAnim();

   uint32_t m_matrixNum = 0;

   int m_channelBase = 0;
   int m_numLocalViews = 0, m_numClusterViews = 0;
   NodeConfig m_localConfig;
   std::vector<NodeConfig> m_nodeConfig;

   opencover::MultiChannelDrawer::Mode m_configuredMode = opencover::MultiChannelDrawer::ReprojectMesh;
   opencover::MultiChannelDrawer::Mode m_mode = opencover::MultiChannelDrawer::AsIs;
   GeometryMode m_configuredGeoMode = RemoteConnection::Screen;
   GeometryMode m_geoMode = RemoteConnection::Invalid;
   ViewSelection m_configuredVisibleViews = opencover::MultiChannelDrawer::Same;
   ViewSelection m_visibleViews = opencover::MultiChannelDrawer::All;

   opencover::ui::Menu *m_menu = nullptr;

   bool m_noModelUpdate = false;
   osg::Matrix m_oldModelMatrix;

   RemotesMap m_remotes;
   std::map<std::string, bool> m_coverVariants; //< whether a Variant is visible
   std::map<int, VistleInteractor *> m_interactors;
   void setServerParameters(int module, const std::string &host, unsigned short port) const;

   void setGeometryMode(GeometryMode mode);
   void setVisibleViews(ViewSelection selection);
   void setReprojectionMode(opencover::MultiChannelDrawer::Mode reproject);
   double m_imageQuality = 1.0;
   int m_maxTilesPerFrame = 100;
   bool m_printViewSizes = true;
};
#endif
