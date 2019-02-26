#ifndef REMOTECONNECTION_H
#define REMOTECONNECTION_H

#include <string>
#include <mutex>
#include <thread>
#include <deque>
#include <map>
#include <vector>
#include <memory>

#include <boost/asio/ip/tcp.hpp>

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Matrix>

#include <PluginUtil/MultiChannelDrawer.h>

#include <rhr/rfbext.h>
#include <renderer/renderobject.h>

#include "TileMessage.h"
#include "NodeConfig.h"

#include <boost/mpi.hpp>

namespace opencover {
class MultiChannelDrawer;
}

class VariantRenderObject;
class RhrClient;
struct DecodeTask;

class RemoteConnection {
    public:

   enum GeometryMode {
       Screen,
       CubeMap,
       CubeMapFront,
       CubeMapCoarseSides,
       Invalid,
   };

    using ViewSelection = opencover::MultiChannelDrawer::ViewSelection;

    static int numViewsForMode(GeometryMode mode);

    RhrClient *plugin = nullptr;
    //! store results of server bounding sphere query
    bool m_boundsUpdated = false;
    osg::ref_ptr<osg::Node> m_boundsNode;
    const osg::BoundingSphere &getBounds() const;

    bool m_listen;
    std::string m_host;
    unsigned short m_port = 0;
    unsigned short m_portFirst = 0, m_portLast = 0;
    bool m_findPort = false;
    boost::asio::ip::tcp::socket m_sock;
    std::unique_ptr<std::recursive_mutex> m_mutex, m_sendMutex;
    std::unique_ptr<std::mutex> m_taskMutex;
    std::unique_ptr<std::thread> m_thread;
    bool m_running = false;
    bool m_listening = false;
    bool m_connected = false;
    bool m_addDrawer = false;
    bool m_interrupt = false;
    bool m_isMaster = false;
    bool m_setServerParameters = false;
    int m_moduleId = 0;
    std::deque<TileMessage> m_receivedTiles;
    std::deque<size_t> m_lastTileAt;
    std::map<std::string, vistle::RenderObject::InitialVariantVisibility> m_variantsToAdd;
    std::set<std::string> m_variantsToRemove;
    std::map<std::string, std::shared_ptr<VariantRenderObject>> m_variants;
    std::map<std::string, bool> m_variantVisibility;
    int m_requestedTimestep=-1, m_remoteTimestep=-1, m_visibleTimestep=-1, m_numRemoteTimesteps=-1;

    int m_channelBase = 0;
    int m_numViews = 0, m_numLocalViews = 0;
    int m_numClusterViews = 0;
    std::vector<NodeConfig> m_nodeConfig;
    osg::ref_ptr<opencover::MultiChannelDrawer> m_drawer;
    opencover::MultiChannelDrawer::Mode m_mode;
    GeometryMode m_geoMode = Invalid;
    opencover::MultiChannelDrawer::ViewSelection m_visibleViews = opencover::MultiChannelDrawer::Same;

    RemoteConnection() = delete;
    RemoteConnection(const RemoteConnection& other) = delete;
    RemoteConnection &operator=(RemoteConnection &other) = delete;
    RemoteConnection(RhrClient *plugin, std::string host, unsigned short port, bool isMaster);
    RemoteConnection(RhrClient *plugin, unsigned short port, bool isMaster);
    RemoteConnection(RhrClient *plugin, unsigned short portFirst, unsigned short portLast, bool isMaster);
    ~RemoteConnection();
    void init();
    void start();

    void setName(const std::string &name);
    const std::string &name() const;

    void stopThread();
    void operator()();
    void connectionEstablished();
    void connectionClosed();
    bool requestTimestep(int t, int numTime=-1);
    bool setLights(const vistle::lightsMsg &msg);
    vistle::lightsMsg m_lights;
    bool setMatrices(const std::vector<vistle::matricesMsg> &msgs, bool force=false);
    std::vector<vistle::matricesMsg> m_matrices, m_savedMatrices;
    int m_numMatrixRequests = 0;
    bool handleAnimation(const vistle::message::RemoteRenderMessage &msg, const vistle::animationMsg &anim);
    bool handleVariant(const vistle::message::RemoteRenderMessage &msg, const vistle::variantMsg &variant);
    bool update();
    void preFrame();
    void updateVariants();
    void setVariantVisibility(const std::string &variant, bool visible);

    //! handle RFB bounds message
    bool handleBounds(const vistle::message::RemoteRenderMessage &msg, const vistle::boundsMsg &bound);
    bool handleTile(const vistle::message::RemoteRenderMessage &msg, const vistle::tileMsg &tile, std::shared_ptr<std::vector<char>> payload);
    bool isRunning() const;
    bool isListening() const;
    bool isConnecting() const;
    bool isConnected() const;
    bool boundsUpdated();
    bool sendMessage(const vistle::message::Message &msg, const std::vector<char> *payload=nullptr);

    template<class Message>
    bool send(const Message &msg, const std::vector<char> *payload=nullptr) {
        vistle::message::RemoteRenderMessage r(msg, payload ? payload->size() : 0);
        return sendMessage(r, payload);
    }

    bool requestBounds();
    void lock();
    void unlock();

    void setNumLocalViews(int nv);
    void setNumClusterViews(int nv);
    void setNumChannels(const std::vector<int> &numChannels);
    void setNodeConfigs(const std::vector<NodeConfig> &configs);
    void setFirstView(int v);
    void setGeometryMode(GeometryMode mode);
    void setReprojectionMode(opencover::MultiChannelDrawer::Mode mode);
    void setViewsToRender(opencover::MultiChannelDrawer::ViewSelection selection);
    void gatherTileStats(const vistle::message::RemoteRenderMessage &remote, const vistle::tileMsg &msg);
    void handleTileMeta(const vistle::message::RemoteRenderMessage &remote, const vistle::tileMsg &msg);

    void finishFrame(const vistle::message::RemoteRenderMessage &msg);

    // statistics and timings
   bool m_benchmark = false;
   double m_lastFrameTime = -1.;
   double m_lastMatricesTime = -1.;
   double m_minDelay = 0., m_maxDelay = 0., m_accumDelay = 0., m_avgDelay = 0.;
   double m_lastStat = -1.;
   size_t m_remoteFrames=0, m_localFrames=0;
   size_t m_depthBytes=0, m_rgbBytes=0, m_depthBpp=0, m_numPixels=0;
   size_t m_depthBytesS=0, m_rgbBytesS=0, m_depthBppS=0, m_numPixelsS=0;
   int m_remoteSkipped = 0;
   std::string m_name;

   bool canEnqueue() const;
   void enqueueTask(std::shared_ptr<DecodeTask> task);
   int m_deferredFrames = 0;
   bool updateTileQueue();
   bool handleTileMessage(std::shared_ptr<const vistle::message::RemoteRenderMessage> msg, std::shared_ptr<std::vector<char>> payload);
   int m_queuedTiles = 0;

   bool m_frameReady = false;
   bool m_waitForFrame = false;
   typedef std::deque<std::shared_ptr<DecodeTask>> TaskQueue;
   TaskQueue m_queuedTasks, m_finishedTasks;
   std::set<std::shared_ptr<DecodeTask>> m_runningTasks;

   bool checkSwapFrame();
   void swapFrame();
   void processMessages();
   void setVisibleTimestep(int t);

   int numTimesteps() const;
   int currentTimestep() const;
   int timestepToCommit() const;

   osg::Group *scene();
   osg::ref_ptr<osg::Group> m_scene;
   void printStats();

   unsigned m_maxTilesPerFrame = 100;
   bool m_handleTilesAsync = false;

   std::unique_ptr<boost::mpi::communicator> m_comm;
   std::unique_ptr<boost::mpi::communicator> m_commAny, m_commMiddle, m_commLeft, m_commRight;
   bool m_needUpdate = false;
   osg::Matrix m_head, m_newHead, m_receivingHead;
   const osg::Matrix &getHeadMat() const;
   bool distributeAndHandleTileMpi(std::shared_ptr<vistle::message::RemoteRenderMessage> msg, std::shared_ptr<std::vector<char> > payload);
};

#endif
