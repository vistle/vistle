#include "RemoteConnection.h"
#include "RhrClient.h"

#include <sstream>
#include <iomanip>
#include <iostream>

#include <VistlePluginUtil/VistleRenderObject.h>

#include <config/CoviseConfig.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coVRConfig.h>
#include <cover/coVRMSController.h>
#include <cover/coVRPluginList.h>
#include <cover/coVRPluginSupport.h>
#include <cover/VRViewer.h>

#include <vistle/core/messages.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/enum.h>
#include <vistle/util/threadname.h>

#include <osg/io_utils>

#include "DecodeTask.h"

//#define CONNDEBUG

#define CERR std::cerr << "RemoteConnection: "
#define NOTIFY_INFO CERR
#define NOTIFY_ERROR CERR
//#define NOTIFY_INFO cover->notify(Notify::Info) << "RhrClient: "
//#define NOTIFY_ERROR cover->notify(Notify::Error) << "RhrClient: "

namespace asio = boost::asio;

typedef std::lock_guard<std::recursive_mutex> lock_guard;

using namespace opencover;
using namespace vistle;
using message::RemoteRenderMessage;

// clang-format off
DEFINE_ENUM_WITH_STRING_CONVERSIONS(MpiTags,
                                    (TagQuit)
                                    (TagContinue)
                                    (TagTileAll)
                                    (TagTileAny)
                                    (TagTileMiddle)
                                    (TagTileLeft)
                                    (TagTileRight)
                                    (TagTileSend)
                                    (TagData)
)
// clang-format on

int RemoteConnection::numViewsForMode(RemoteConnection::GeometryMode mode)
{
    switch (mode) {
    case Screen:
        return -1;
    case FirstScreen:
    case OnlyFront:
        return 1;
    case CubeMap:
        return 6;
    case CubeMapFront:
    case CubeMapCoarseSides:
        return 5;
    case Invalid:
        return 0;
    }

    return 0;
}

const osg::BoundingSphere &RemoteConnection::getBounds() const
{
    lock_guard locker(*m_mutex);
    return m_boundsNode->getBound();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, int moduleId, bool isMaster)
: plugin(plugin), m_sock(plugin->m_io), m_isMaster(isMaster), m_moduleId(moduleId)
{
    init();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, std::string host, unsigned short port, bool isMaster,
                                   const std::string &tunnelId)
: plugin(plugin), m_host(host), m_port(port), m_tunnelId(tunnelId), m_sock(plugin->m_io), m_isMaster(isMaster)
{
    init();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, unsigned short portFirst, unsigned short portLast, bool isMaster)
: plugin(plugin)
, m_port(0)
, m_portFirst(portFirst)
, m_portLast(portLast)
, m_sock(plugin->m_io)
, m_listen(true)
, m_isMaster(isMaster)
, m_setServerParameters(true)
{
    if (m_portFirst >= m_portLast) {
        m_port = m_portFirst;
        m_portFirst = m_portLast = 0;

        m_setServerParameters = false;
    }

    init();
}

RemoteConnection::~RemoteConnection()
{
    if (!m_thread) {
        assert(!m_sock.is_open());
    } else {
        stopThread();
        m_thread->join();
        m_thread.reset();
        NOTIFY_INFO << "disconnected from server" << std::endl;
    }

    while (!m_runningTasks.empty())
        usleep(1000);
}

void RemoteConnection::init()
{
    const std::string conf("COVER.Plugin.RhrClient");

    bool exists = false;
    bool useMpi = covise::coCoviseConfig::isOn("mpi", conf, false, &exists);
    m_handleTilesAsync = useMpi && covise::coCoviseConfig::isOn("thread", conf, m_handleTilesAsync, &exists);

    CERR << "new " << (m_listen ? "listening" : "client") << " RemoteConnection ";
    if (m_moduleId != message::Id::Invalid) {
        std::cerr << "to module " << m_moduleId;
    }
    if (!m_tunnelId.empty()) {
        std::cerr << " with tunnelId " << m_tunnelId;
    }
    std::cerr << std::endl;
#ifdef CONNDEBUG
    CERR << "init: mpi=" << useMpi << ", thread=" << m_handleTilesAsync << std::endl;
#endif

    if (coVRMSController::instance()->isCluster() && useMpi) {
        m_comm.reset(new boost::mpi::communicator(coVRMSController::instance()->getAppCommunicator(),
                                                  boost::mpi::comm_duplicate));
    }
    if (m_comm) {
        if (m_handleTilesAsync) {
            CERR << "handling tiles and MPI communication on separate thread" << std::endl;
        } else {
            CERR << "off-loading MPI communication to dedicated thread" << std::endl;
        }
    }

    m_boundsNode = new osg::Node;
    m_mutex.reset(new std::recursive_mutex);
    m_sendMutex.reset(new std::recursive_mutex);
    m_taskMutex.reset(new std::mutex);

    m_drawer = new MultiChannelDrawer(true /* flip top/bottom */);
    m_drawer->setName("RemoteConnectionDrawer");

    m_scene = new osg::Group;
    m_scene->setName("RemoteConnection");

    m_head = m_newHead = m_receivingHead = cover->getViewerMat();

    m_status = "initialized";

    m_initialized = true;

    m_tunnelEstablished = m_tunnelId.empty();
}

void RemoteConnection::startThread()
{
    assert(m_initialized);

    if (m_handleTilesAsync || m_isMaster) {
        m_running = true;
        m_thread.reset(new std::thread(std::ref(*this)));
        m_status = "running";
    }
}

void RemoteConnection::setName(const std::string &name)
{
    m_name = name;
}

const std::string &RemoteConnection::name() const
{
    return m_name;
}

void RemoteConnection::stopThread()
{
    {
        lock_guard locker(*m_mutex);
        m_interrupt = true;
    }
    {
        lock_guard locker(*m_mutex);
        if (m_sock.is_open()) {
            boost::system::error_code ec;
            m_sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        }
    }

    for (;;) {
        {
            lock_guard locker(*m_mutex);
            if (!m_running)
                break;
        }
        usleep(10000);
    }

    lock_guard locker(*m_mutex);
    assert(!m_running);

    m_status = "stopped";
}

void RemoteConnection::operator()()
{
    vistle::setThreadName("RHR:RemoteConnection");

    if (m_comm)
        CERR << "starting thread on rank " << m_comm->rank() << std::endl;
    else
        CERR << "starting thread on master" << std::endl;
    {
        lock_guard locker(*m_mutex);
        assert(m_running);
        m_listening = false;
    }

    if (!m_isMaster) {
        assert(m_handleTilesAsync);
        assert(m_comm);

        std::shared_ptr<RemoteRenderMessage> msg;
        std::shared_ptr<vistle::buffer> payload;
        while (distributeAndHandleTileMpi(msg, payload)) {
            lock_guard locker(*m_mutex);
            if (m_interrupt) {
                break;
            }
        }

        CERR << "stopping thread on rank " << m_comm->rank() << std::endl;
        lock_guard locker(*m_mutex);
        m_running = false;
        return;
    }

#define END(s) \
    if (m_comm) \
        for (int i = 1; i < m_comm->size(); ++i) { \
            m_comm->send(i, TagQuit); \
        } \
    lock_guard locker(*m_mutex); \
    m_running = false; \
    m_status = s; \
    return;

    if (m_moduleId != message::Id::Invalid) {
        connectionEstablished();
    } else if (m_listen) {
        boost::system::error_code ec;
        asio::ip::tcp::acceptor acceptorv4(plugin->m_io), acceptorv6(plugin->m_io);
        int first = m_port, last = m_port;
        if (m_portFirst > 0 && m_portLast > 0) {
            first = m_portFirst;
            last = m_portLast;
        }
        asio::ip::tcp::endpoint endpoint;
        for (m_port = first; m_port <= last; ++m_port) {
            if (start_listen(m_port, acceptorv4, acceptorv6, ec)) {
                break;
            }
            if (ec != boost::system::errc::address_in_use) {
                break;
            }
        }
        if (ec) {
            NOTIFY_ERROR << "could not open port " << m_port << " for listening: " << ec.message() << std::endl;
            END("port error");
        }
        ec.clear();
        if (acceptorv4.is_open())
            acceptorv4.non_blocking(true, ec);
        if (!ec) {
            if (acceptorv6.is_open())
                acceptorv6.non_blocking(true, ec);
        }
        if (ec) {
            NOTIFY_ERROR << "could not make acceptor non-blocking: " << ec.message() << std::endl;
            acceptorv4.close();
            acceptorv6.close();
            END("acceptor error");
        }
        assert(acceptorv4.is_open() || acceptorv6.is_open());
        CERR << "listening on port " << m_port << std::endl;
        {
            lock_guard locker(*m_mutex);
            m_listening = true;
            m_status = "listening";
        }

        do {
            if (acceptorv4.is_open()) {
                acceptorv4.accept(m_sock, ec);
                if (ec != boost::system::errc::operation_would_block &&
                    ec != boost::system::errc::resource_unavailable_try_again) {
                    break;
                }
            }
            if (acceptorv6.is_open()) {
                acceptorv6.accept(m_sock, ec);
                if (ec != boost::system::errc::operation_would_block &&
                    ec != boost::system::errc::resource_unavailable_try_again) {
                    break;
                }
            }
            {
                lock_guard locker(*m_mutex);
                if (m_interrupt) {
                    break;
                }
            }
            usleep(1000);
        } while (ec == boost::system::errc::operation_would_block ||
                 ec == boost::system::errc::resource_unavailable_try_again);
        acceptorv4.close();
        acceptorv6.close();
        if (ec) {
            NOTIFY_ERROR << "failure to accept client on port " << m_port << ": " << ec.message() << std::endl;
            END("accept error");
        } else {
            sendMessage(message::Identify());
        }
    } else {
        asio::ip::tcp::resolver resolver(plugin->m_io);
        asio::ip::tcp::resolver::query query(m_host, std::to_string(m_port),
                                             asio::ip::tcp::resolver::query::numeric_service);
        boost::system::error_code ec;
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec) {
            NOTIFY_ERROR << "could not resolve " << m_host << ": " << ec.message() << std::endl;
            END("resolve error");
        }
        asio::connect(m_sock, endpoint_iterator, ec);
        if (ec) {
            NOTIFY_ERROR << "could not establish connection to " << m_host << ":" << m_port << ": " << ec.message()
                         << std::endl;
            END("connection error");
        }
    }

    {
        lock_guard locker(*m_mutex);
        m_listening = false;
        m_connected = true;
        m_status = "connected";
    }
#if 0
    if (m_moduleId == message::Id::Invalid && !m_listen) {
        if (!m_tunnelId.empty()) {
            sendMessage(message::Identify());
        }
    }
#endif

    for (;;) {
        if (m_moduleId != message::Id::Invalid) {
        } else if (!m_sock.is_open()) {
            lock_guard locker(*m_mutex);
            m_connected = false;
            m_status = "disconnected";
            connectionClosed();
            break;
        }

        message::Buffer buf;
        auto payload = std::make_shared<buffer>();
        message::error_code ec;
        bool received = false;
        if (m_moduleId == message::Id::Invalid) {
            received = message::recv(m_sock, buf, ec, false, payload.get());
            if (ec) {
                NOTIFY_ERROR << "message receive: " << ec.message() << std::endl;
                lock_guard locker(*m_mutex);
                m_status = "receive error";
                break;
            }
        }
        {
            lock_guard locker(*m_mutex);
            if (m_interrupt) {
                break;
            }
        }
        if (!received) {
            usleep(10000);
            continue;
        }

        if (buf.type() == message::TUNNELESTABLISHED) {
            auto &msg = buf.as<message::TunnelEstablished>();
            CERR << "tunnel established: " << msg << std::endl;
            lock_guard locker(*m_mutex);
            m_tunnelEstablished = true;
            m_status = "tunnel established";
            if (msg.role() == message::TunnelEstablished::Server) {
                sendMessage(message::Identify());
            }
        } else if (buf.type() == message::IDENTIFY) {
            auto &msg = buf.as<message::Identify>();
            CERR << "received identify: " << msg << std::endl;
            using message::Identify;
            switch (msg.identity()) {
            case Identify::REQUEST: {
                if (m_tunnelEstablished) {
                    message::Identify id(msg, message::Identify::RENDERCLIENT);
                    lock_guard locker(*m_mutex);
                    sendMessage(id);
                    connectionEstablished();
                    m_status = "connected";
                } else {
                    message::Identify id(msg, m_tunnelId, Identify::Client);
                    lock_guard locker(*m_mutex);
                    sendMessage(id);
                    m_status = "tunnel establishment";
                }
                break;
            }
            case Identify::RENDERSERVER: {
                if (!msg.verifyMac()) {
                    lock_guard locker(*m_mutex);
                    m_status = "MAC error";
                    m_interrupt = true;
                    break;
                }
                lock_guard locker(*m_mutex);
                connectionEstablished();
                break;
            }
            default: {
                CERR << "invalid identity: " << msg << std::endl;
                lock_guard locker(*m_mutex);
                m_status = "remote error";
                m_interrupt = true;
                break;
            }
            }

        } else if (buf.type() != message::REMOTERENDERING) {
            CERR << "invalid message type=" << buf.type() << " received" << std::endl;
            lock_guard locker(*m_mutex);
            m_status = "protocol error";
            m_interrupt = true;
            break;
        } else {
            auto &msg = buf.as<RemoteRenderMessage>();
            handleRemoteRenderMessage(msg, payload);
        }

        lock_guard locker(*m_mutex);
        if (m_interrupt) {
            break;
        }
    }

    if (m_comm) {
        for (int i = 1; i < m_comm->size(); ++i)
            m_comm->send(i, TagQuit);
    }
    CERR << "terminating rank 0 with status " << m_status << std::endl;
    lock_guard locker(*m_mutex);
    m_running = false;
}

bool RemoteConnection::handleRemoteRenderMessage(message::RemoteRenderMessage &msg,
                                                 const std::shared_ptr<buffer> &payload)
{
    assert(m_initialized);

    auto &rhr = msg.rhr();
    m_modificationCount = rhr.modificationCount;
    switch (rhr.type) {
    case rfbTile: {
        return handleTile(msg, static_cast<const tileMsg &>(rhr), payload);
    }
    case rfbBounds: {
        lock_guard locker(*m_mutex);
        return handleBounds(msg, static_cast<const boundsMsg &>(rhr));
    }
    case rfbAnimation: {
        lock_guard locker(*m_mutex);
        return handleAnimation(msg, static_cast<const animationMsg &>(rhr));
    }
    case rfbVariant: {
        lock_guard locker(*m_mutex);
        return handleVariant(msg, static_cast<const variantMsg &>(rhr));
    }
    }

    return false;
}

void RemoteConnection::connectionEstablished()
{
    setVisibleTimestep(m_visibleTimestep);
    if (m_requestedTimestep != -1) {
        requestTimestep(m_requestedTimestep);
    }

    for (const auto &var: m_variantVisibility) {
        variantMsg msg;
        strncpy(msg.name, var.first.c_str(), sizeof(msg.name) - 1);
        msg.name[sizeof(msg.name) - 1] = '\0';
        msg.visible = var.second;
        send(msg);
    }

    send(m_lights);
    for (auto &m: m_matrices)
        send(m);

    m_addDrawer = true;
}

void RemoteConnection::connectionClosed()
{}

bool RemoteConnection::requestTimestep(int t, int numTime)
{
    lock_guard locker(*m_mutex);

#ifdef CONNDEBUG
    CERR << "requesting timestep=" << t << " of " << numTime << " (remote: " << m_numRemoteTimesteps << ")"
         << std::endl;
#endif
    m_requestedTimestep = t;
    t = m_numRemoteTimesteps > t ? t : -1;
    animationMsg msg;
    msg.current = t;
    msg.time = cover->frameTime();
    if (numTime == -1)
        msg.total = coVRAnimationManager::instance()->getNumTimesteps();
    else
        msg.total = numTime;
    return send(msg);
}

bool RemoteConnection::setLights(const lightsMsg &msg)
{
    lock_guard locker(*m_mutex);

    if (m_lights != msg) {
        m_lights = msg;
        return send(msg);
    }

    return true;
}

bool RemoteConnection::setMatrices(const std::vector<matricesMsg> &msgs, bool force)
{
    lock_guard locker(*m_mutex);

    if (m_matrices != msgs) {
        m_savedMatrices = msgs;

        auto dt = cover->frameTime() - m_lastMatricesTime;
        if (!force && dt < std::min(0.1, m_avgDelay * 0.5)) {
            return true;
        }

        m_matrices = msgs;

        if (!m_matrices.empty())
            m_matrices.back().last = 1;

        ++m_numMatrixRequests;
        for (auto &m: m_matrices)
            m.requestNumber = m_numMatrixRequests;

        for (auto &m: m_matrices)
            if (!send(m))
                return false;

        m_lastMatricesTime = cover->frameTime();
    }

    return true;
}

bool RemoteConnection::handleAnimation(const RemoteRenderMessage &msg, const animationMsg &anim)
{
    m_numRemoteTimesteps = std::max(0, anim.total);
    CERR << "Animation: #timesteps=" << anim.total << ", cur=" << anim.current << std::endl;

    if (m_requestedTimestep >= 0)
        requestTimestep(m_requestedTimestep < m_numRemoteTimesteps ? m_requestedTimestep : -1);
    else
        requestTimestep(m_visibleTimestep < m_numRemoteTimesteps ? m_visibleTimestep : -1);

    return true;
}

bool RemoteConnection::handleVariant(const RemoteRenderMessage &msg, const variantMsg &variant)
{
    std::string name = variant.name;
    if (variant.remove) {
        m_variantsToRemove.emplace(name);
        CERR << "Variant: remove " << name << std::endl;
    } else {
        vistle::RenderObject::InitialVariantVisibility vis = vistle::RenderObject::DontChange;
        if (variant.configureVisibility)
            vis = variant.visible ? vistle::RenderObject::Visible : vistle::RenderObject::Hidden;
        m_variantsToAdd.emplace(name, vis);
        CERR << "Variant: add " << name << std::endl;
    }
    return true;
}

// called from OpenCOVER main thread
bool RemoteConnection::update()
{
    setMatrices(m_savedMatrices, m_needUpdate);
    m_needUpdate = false;

    if (!m_handleTilesAsync) {
        processMessages();
    }
    updateTileQueue();

    lock_guard locker(*m_mutex);
    coVRMSController::instance()->syncData(&m_connected, sizeof(m_connected));
    updateVariants();
    coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
    coVRMSController::instance()->syncData(&m_remoteTimestep, sizeof(m_remoteTimestep));

    return m_frameReady;
}

// called from OpenCOVER main thread
void RemoteConnection::preFrame()
{
    bool addDrawer = false, connected = false;
    {
        lock_guard locker(*m_mutex);
        coVRMSController::instance()->syncData(&m_addDrawer, sizeof(m_addDrawer));
        addDrawer = m_addDrawer;
        connected = m_connected;
        if (m_connected)
            m_addDrawer = false;
    }

    if (addDrawer) {
        m_scene->addChild(m_drawer);
    } else if (!connected) {
        m_scene->removeChild(m_drawer);
    }

    m_drawer->update();
}

void RemoteConnection::drawFinished()
{
#ifdef CONNDEBUG
    if (!m_frameDrawn) {
        CERR << "*** drawFinished ***" << std::endl;
    }
#endif
    std::lock_guard<std::mutex> locker(*m_taskMutex);
    m_frameDrawn = true;
}

void RemoteConnection::updateVariants()
{
    unsigned numAdd = m_variantsToAdd.size(), numRemove = m_variantsToRemove.size();
    coVRMSController::instance()->syncData(&numAdd, sizeof(numAdd));
    coVRMSController::instance()->syncData(&numRemove, sizeof(numRemove));
    auto itAdd = m_variantsToAdd.begin();
    for (unsigned i = 0; i < numAdd; ++i) {
        std::string s;
        enum vistle::RenderObject::InitialVariantVisibility vis;
        if (m_isMaster) {
            s = itAdd->first;
            vis = itAdd->second;
        }
        s = coVRMSController::instance()->syncString(s);
        coVRMSController::instance()->syncData(&vis, sizeof(vis));
        if (!m_isMaster) {
            m_variantsToAdd.emplace(s, vis);
        } else {
            ++itAdd;
        }
    }
    assert(m_variantsToAdd.size() == numAdd);
    auto itRemove = m_variantsToRemove.begin();
    for (unsigned i = 0; i < numRemove; ++i) {
        std::string s;
        if (m_isMaster)
            s = *itRemove;
        s = coVRMSController::instance()->syncString(s);
        if (!m_isMaster) {
            m_variantsToRemove.insert(s);
        } else {
            ++itRemove;
        }
    }
    assert(m_variantsToRemove.size() == numRemove);

    if (!m_variantsToAdd.empty())
        cover->addPlugin("Variant");
    for (auto &var: m_variantsToAdd) {
        auto it = m_variants.find(var.first);
        if (it == m_variants.end()) {
            it = m_variants.emplace(var.first, std::make_shared<VariantRenderObject>(var.first, var.second)).first;
        }
        coVRPluginList::instance()->addNode(it->second->node(), it->second.get(), plugin);
    }
    m_variantsToAdd.clear();

    for (auto &var: m_variantsToRemove) {
        auto it = m_variants.find(var);
        if (it != m_variants.end()) {
            coVRPluginList::instance()->removeNode(it->second->node(), it->second.get());
            m_variants.erase(it);
        }
    }
    m_variantsToRemove.clear();
}

void RemoteConnection::setVariantVisibility(const std::string &variant, bool visible)
{
    lock_guard locker(*m_mutex);
    m_variantVisibility[variant] = visible;

    if (m_isMaster && isConnected()) {
        variantMsg msg;
        strncpy(msg.name, variant.c_str(), sizeof(msg.name) - 1);
        msg.name[sizeof(msg.name) - 1] = '\0';
        msg.visible = visible;
        send(msg);
    }
}

bool RemoteConnection::handleBounds(const RemoteRenderMessage &msg, const boundsMsg &bound)
{
    //CERR << "receiving bounds: center: " << bound.center[0] << " " << bound.center[1] << " " << bound.center[2] << ", radius: " << bound.radius << std::endl;

    m_boundsUpdated = true;
    m_boundsNode->setInitialBound(
        osg::BoundingSphere(osg::Vec3(bound.center[0], bound.center[1], bound.center[2]), bound.radius));
    m_boundsModificationCount = bound.modificationCount;

    //osg::BoundingSphere bs = boundsNode->getBound();
    //CERR << "server bounds: " << bs.center() << ", " << bs.radius() << std::endl;

    return true;
}

bool RemoteConnection::handleTile(const RemoteRenderMessage &msg, const tileMsg &tile, std::shared_ptr<buffer> payload)
{
    assert(msg.rhr().type == rfbTile);
#ifdef CONNDEBUG
    std::string type = tile.format == rfbColorRGBA ? "C" : "Z";
    CERR << "TILE recv " << (tile.flags & rfbTileFirst ? "F" : ".") << (tile.flags & rfbTileLast ? "L" : ".")
         << " req=" << tile.requestNumber << " frame=" << tile.frameNumber
         << (tile.format == rfbColorRGBA ? " C" : " Z") << ", view=" << tile.viewNum << ": " << tile.width << "x"
         << tile.height << "@" << tile.x << "+" << tile.y << " t=" << tile.timestep << std::endl;
#endif

    if (tile.size == 0 && tile.width == 0 && tile.height == 0 && tile.flags == 0) {
        CERR << "received initial tile - ignoring" << std::endl;
        return true;
    }

    assert(payload);
    assert(payload->size() == msg.payloadSize());
    auto m = std::make_shared<RemoteRenderMessage>(msg);
    if (m_handleTilesAsync) {
        m_receivedTiles.push_back(TileMessage(m, payload));
        if (tile.flags & rfbTileLast)
            m_lastTileAt.push_back(m_receivedTiles.size());
        skipFrames();
        unsigned ntiles = m_receivedTiles.size();
        if (!m_lastTileAt.empty()) {
            ntiles = m_lastTileAt.front();
            assert(ntiles <= m_receivedTiles.size());
            m_lastTileAt.pop_front();
        }
        bool process = false;
        {
            std::lock_guard<std::mutex> locker(*m_taskMutex);
            process = canEnqueue();
        }
        if (process) {
            for (unsigned i = 0; i < ntiles; ++i) {
                auto &t = m_receivedTiles.front();
                if (m_comm) {
                    distributeAndHandleTileMpi(t.msg, t.payload);
                } else {
                    handleTileMessage(t.msg, t.payload);
                }
                m_receivedTiles.pop_front();
            }
            for (auto &l: m_lastTileAt) {
                l -= ntiles;
            }
        }
        return true;
    }

    lock_guard locker(*m_mutex);
    m_receivedTiles.push_back(TileMessage(m, payload));
    if (tile.flags & rfbTileLast)
        m_lastTileAt.push_back(m_receivedTiles.size());

    return true;
}

void RemoteConnection::gatherTileStats(const RemoteRenderMessage &remote, const tileMsg &msg)
{
    if (msg.flags & rfbTileFirst) {
        m_numPixels = 0;
        m_depthBytes = 0;
        m_rgbBytes = 0;
    }
    if (msg.format != rfbColorRGBA) {
        m_depthBytes += remote.payloadSize();
    } else {
        m_rgbBytes += remote.payloadSize();
        m_numPixels += msg.width * msg.height;
    }
}

void RemoteConnection::handleTileMeta(const RemoteRenderMessage &remote, const tileMsg &msg)
{
#ifdef CONNDEBUG
    CERR << "TILE meta " << (msg.flags & rfbTileFirst ? "F" : ".") << (msg.flags & rfbTileLast ? "L" : ".")
         << " req=" << msg.requestNumber << " frame=" << msg.frameNumber << (msg.format == rfbColorRGBA ? " C" : " Z")
         << ", view=" << msg.viewNum << ": " << msg.width << "x" << msg.height << "@" << msg.x << "+" << msg.y
         << " t=" << msg.timestep << std::endl;

#if 0
    bool first = msg.flags & rfbTileFirst;
    bool last = msg.flags & rfbTileLast;
    if (first || last) {
        CERR << "TILE: " << (first ? "F" : "f") << (last ? "L" : "l") << ", t=" << msg.timestep
             << "req: " << msg.requestNumber << ", view: " << msg.viewNum << ", frame: " << msg.frameNumber
             << ", dt: " << cover->frameTime() - msg.requestTime << std::endl;
    }
#endif
#endif
    if (currentFrame >= 0 && currentFrame > msg.frameNumber) {
        CERR << "receiving tiles out of order: expectecd at least frame no. " << currentFrame << ", but got "
             << msg.frameNumber << std::endl;
    }
    assert(currentFrame < 0 || msg.frameNumber >= currentFrame);
    if (expectNewFrame && msg.frameNumber == currentFrame) {
        CERR << "receiving tile after last tile for frame " << currentFrame << ", but got " << msg.frameNumber
             << std::endl;
    }
    assert(!expectNewFrame || msg.frameNumber > currentFrame);
    expectNewFrame = msg.flags & rfbTileLast;
    currentFrame = msg.frameNumber;
    osg::Matrix model, view, proj, head;
    for (int i = 0; i < 16; ++i) {
        head.ptr()[i] = msg.head[i];
        view.ptr()[i] = msg.view[i];
        proj.ptr()[i] = msg.proj[i];
        model.ptr()[i] = msg.model[i];
    }
    int viewIdx = msg.viewNum;
    if (m_geoMode == Screen && m_visibleViews == MultiChannelDrawer::Same) {
        viewIdx -= m_channelBase;
    }
    if (viewIdx < 0 || viewIdx >= m_numViews) {
        if (!msg.flags)
            CERR << "reject tile with viewIdx=" << viewIdx << ", base=" << m_channelBase << ", numViews=" << m_numViews
                 << std::endl;
        return;
    }

    m_receivingHead = head;

    m_drawer->updateMatrices(viewIdx, model, view, proj);
    int w = msg.totalwidth, h = msg.totalheight;

    if (msg.format == rfbColorRGBA) {
        m_drawer->resizeView(viewIdx, w, h);
    } else {
        GLenum format = GL_FLOAT;
        // FIXME: not yet
        //m_drawer->setLinearDepth(msg.format == rfbDepthViewer);
        switch (msg.format) {
        case rfbDepthFloat:
            format = GL_FLOAT;
            m_depthBpp = 4;
            break;
        case rfbDepthViewer:
            format = GL_FLOAT;
            m_depthBpp = 4;
            break;
        case rfbDepth8Bit:
            format = GL_UNSIGNED_BYTE;
            m_depthBpp = 1;
            break;
        case rfbDepth16Bit:
            format = GL_UNSIGNED_SHORT;
            m_depthBpp = 2;
            break;
        case rfbDepth24Bit:
            format = GL_FLOAT;
            m_depthBpp = 4;
            break;
        case rfbDepth32Bit:
            format = GL_FLOAT;
            m_depthBpp = 4;
            break;
        default:
            CERR << "unhandled image format " << msg.format << std::endl;
            return;
        }
        m_drawer->resizeView(viewIdx, w, h, format, 0);
    }
}


bool RemoteConnection::isRunning() const
{
    lock_guard locker(*m_mutex);
    return m_running;
}

bool RemoteConnection::isListener() const
{
    lock_guard locker(*m_mutex);
    return m_listen;
}

bool RemoteConnection::isListening() const
{
    lock_guard locker(*m_mutex);
    if (m_isMaster) {
        return m_running && m_listening;
    } else {
        return m_listen;
    }
}

bool RemoteConnection::isConnecting() const
{
    lock_guard locker(*m_mutex);
    return m_running && !m_connected && m_sock.is_open();
}

bool RemoteConnection::isConnected() const
{
    lock_guard locker(*m_mutex);
    if (m_moduleId != message::Id::Invalid) {
        return true;
    }
    return m_running && m_connected && m_sock.is_open();
}

bool RemoteConnection::boundsUpdated()
{
    lock_guard locker(*m_mutex);
    bool ret = m_boundsUpdated;
    m_boundsUpdated = false;
    m_boundsCurrent = true;
    return ret;
}

bool RemoteConnection::sendMessage(const message::Message &msg, const buffer *payload)
{
    lock_guard locker(*m_sendMutex);
    if (!isConnected())
        return false;

    if (m_moduleId != message::Id::Invalid) {
        message::Buffer buf(msg);
        buf.setDestId(m_moduleId);
        return plugin->sendMessage(buf, payload);
    }

    if (!message::send(m_sock, msg, payload)) {
        if (m_sock.is_open()) {
            boost::system::error_code ec;
            m_sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        }
        return false;
    }
    return true;
}

bool RemoteConnection::requestBounds()
{
    lock_guard locker(*m_mutex);
    if (!isConnected())
        return false;
    m_boundsUpdated = false;
    m_boundsCurrent = false;

    boundsMsg msg;
    msg.type = rfbBounds;
    msg.sendreply = 1;
    return send(msg);
}

bool RemoteConnection::boundsCurrent() const
{
    lock_guard locker(*m_mutex);
    return m_boundsCurrent && m_modificationCount == m_boundsModificationCount;
}

void RemoteConnection::lock()
{
    m_mutex->lock();
}

void RemoteConnection::unlock()
{
    m_mutex->unlock();
}

void RemoteConnection::setNumLocalViews(int nv)
{
    m_numLocalViews = nv;
}

void RemoteConnection::setNumClusterViews(int nv)
{
    m_numClusterViews = nv;
}

void RemoteConnection::setNodeConfigs(const std::vector<NodeConfig> &configs)
{
    m_nodeConfig = configs;

    if (m_comm) {
        auto &nc = m_nodeConfig[m_comm->rank()];
        bool master = coVRMSController::instance()->isMaster();

        m_commAny.reset(new boost::mpi::communicator(
            m_comm->split(nc.haveMiddle || nc.haveLeft || nc.haveRight || master ? 1 : MPI_UNDEFINED)));
        m_commMiddle.reset(new boost::mpi::communicator(m_comm->split(nc.haveMiddle || master ? 1 : MPI_UNDEFINED)));
        m_commLeft.reset(new boost::mpi::communicator(m_comm->split(nc.haveLeft || master ? 1 : MPI_UNDEFINED)));
        m_commRight.reset(new boost::mpi::communicator(m_comm->split(nc.haveRight || master ? 1 : MPI_UNDEFINED)));
    }
}

void RemoteConnection::setFirstView(int v)
{
    m_channelBase = v;
}

void RemoteConnection::setGeometryMode(RemoteConnection::GeometryMode mode)
{
    if (m_geoMode == mode)
        return;

    m_geoMode = mode;

    m_numViews = numViewsForMode(mode);

    if (mode == Screen) {
        m_drawer->setViewsToRender(m_visibleViews);
        if (m_visibleViews == MultiChannelDrawer::Same) {
            m_numViews = m_numLocalViews;
            m_drawer->setNumViews(-1);
        } else {
            m_numViews = m_numClusterViews;
            m_drawer->setNumViews(m_numClusterViews);
        }
    } else if (mode == FirstScreen || mode == OnlyFront || mode == CubeMap || mode == CubeMapFront ||
               mode == CubeMapCoarseSides) {
        m_drawer->setViewsToRender(MultiChannelDrawer::All);
        m_drawer->setNumViews(m_numViews);
    } else {
        m_drawer->setNumViews(0);
    }

    m_needUpdate = true;

    CERR << "setGeometryMode(mode=" << mode << "), numViews=" << m_numViews << std::endl;
}

void RemoteConnection::setReprojectionMode(MultiChannelDrawer::Mode mode)
{
    m_mode = mode;
    m_drawer->setMode(mode);
}

void RemoteConnection::setViewsToRender(MultiChannelDrawer::ViewSelection selection)
{
    if (selection == m_visibleViews)
        return;

    m_visibleViews = selection;

    if (m_geoMode != Screen)
        return;

    m_drawer->setViewsToRender(selection);
    if (selection == MultiChannelDrawer::Same) {
        m_numViews = m_numLocalViews;
        m_drawer->setNumViews(-1);
    } else {
        m_numViews = m_numClusterViews;
        m_drawer->setNumViews(m_numClusterViews);
    }

    m_needUpdate = true;

    CERR << "setViewsToRender(state=" << selection << "), numViews=" << m_numViews << std::endl;
}

bool RemoteConnection::canEnqueue() const
{
    return !m_frameReady && !m_waitForFrame && m_frameDrawn;
}

void RemoteConnection::enqueueTask(std::shared_ptr<DecodeTask> task)
{
    assert(canEnqueue());

    const auto &tile = static_cast<const tileMsg &>(task->msg->rhr());
    handleTileMeta(*task->msg, tile);

    const bool first = tile.flags & rfbTileFirst;
    const bool last = tile.flags & rfbTileLast;
    int view = tile.viewNum;
    if (m_geoMode == RemoteConnection::Screen && m_visibleViews == MultiChannelDrawer::Same) {
        view -= m_channelBase;
    }

    if (view < 0 || view >= m_numViews) {
        if (!tile.flags) {
            CERR << "NO FB for view " << view << std::endl;
        }
        task->rgba = NULL;
        task->depth = NULL;
    } else {
        task->viewData = m_drawer->getViewData(view);
        if (tile.format == rfbColorRGBA) {
            task->rgba = reinterpret_cast<char *>(m_drawer->rgba(view));
            task->depth = NULL;
        } else {
            task->rgba = NULL;
            task->depth = reinterpret_cast<char *>(m_drawer->depth(view));
        }

        switch (tile.eye) {
        case rfbEyeMiddle:
            m_drawer->setViewEye(view, Middle);
            break;
        case rfbEyeLeft:
            m_drawer->setViewEye(view, Left);
            break;
        case rfbEyeRight:
            m_drawer->setViewEye(view, Right);
            break;
        }
    }

    if (last) {
#ifdef CONNDEBUG
        CERR << "waiting for frame finish: x=" << tile.x << ", y=" << tile.y << ", #queued=" << m_queuedTiles
             << std::endl;
#endif
        m_waitForFrame = true;
    }

    if (first) {
        assert(m_queuedTiles == 0);
    }
    ++m_queuedTiles;

    m_runningTasks.emplace(task);
    task->result = std::async(std::launch::async, [this, task]() {
        if (task->rgba)
            setThreadName("RHR:Tile:RGBA");
        else
            setThreadName("RHR:Tile:Depth");

        bool ok = task->work();
        std::lock_guard<std::mutex> locker(*m_taskMutex);
        m_finishedTasks.emplace_back(task);
        m_runningTasks.erase(task);
        //CERR << "FIN: #running=" << m_runningTasks.size() << ", #fin=" << m_finishedTasks.size() << std::endl;
        return ok;
    });
}

bool RemoteConnection::handleTileMessage(std::shared_ptr<const message::RemoteRenderMessage> msg,
                                         std::shared_ptr<buffer> payload)
{
    assert(msg->rhr().type == rfbTile);
    const auto &tile = static_cast<const tileMsg &>(msg->rhr());

    gatherTileStats(*msg, tile);

#ifdef CONNDEBUG
    bool first = tile.flags & rfbTileFirst;
    bool last = tile.flags & rfbTileLast;

#if 0
   CERR << "q=" << m_queuedTiles << ", wait=" << m_waitForFrame << ", tile: x="<<tile.x << ", y="<<tile.y << ", w="<<tile.width<<", h="<<tile.height << ", total w=" << tile.totalwidth
      << ", first: " << bool(tile.flags&rfbTileFirst) << ", last: " << last
      << std::endl;
#endif
    CERR << "q=" << m_queuedTiles << ", wait=" << m_waitForFrame << ", tile: first: " << first << ", last: " << last
         << ", req: " << tile.requestNumber << std::endl;
#endif

    if (tile.flags) {
        // meta data for tiles with flags has to be processed on master and all slaves
    } else if (m_geoMode == Screen && m_visibleViews == MultiChannelDrawer::Same) {
        // ignore tiles for other nodes
        if (tile.viewNum < m_channelBase || tile.viewNum >= m_channelBase + m_numLocalViews) {
            //CERR << "RemoteConnection: ignoring tile for foreign view " << tile.viewNum << std::endl;
            return true;
        }
    } else {
        // ignore invalid tiles
        if (tile.viewNum < 0 || tile.viewNum >= m_numViews) {
            CERR << "RemoteConnection: ignoring tile for invalid view " << tile.viewNum << std::endl;
            return true;
        }
    }

    auto task = std::make_shared<DecodeTask>(msg, payload);
    std::lock_guard<std::mutex> locker(*m_taskMutex);
    if (canEnqueue() && m_queuedTasks.empty()) {
        enqueueTask(task);
    } else {
        assert(m_deferredFrames >= 0);
        if (tile.flags & rfbTileFirst)
            ++m_deferredFrames;
        m_queuedTasks.emplace_back(task);
        return false;
    }

    assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
    assert(m_queuedTasks.empty() || m_deferredFrames > 0);

    return true;
}

// returns true if a frame has been discarded - should be called again
bool RemoteConnection::updateTileQueue()
{
    //CERR << "tiles: " << m_queuedTiles << " queued, " << m_queuedTasks.size() << " deferred, " << m_finishedTasks.size() << " finished" << std::endl;
    //assert(m_queuedTiles >= m_finishedTasks.size());
    std::unique_lock<std::mutex> locker(*m_taskMutex);
    while (!m_finishedTasks.empty()) {
        auto dt = m_finishedTasks.front();
        m_finishedTasks.pop_front();

        --m_queuedTiles;
        assert(m_queuedTiles >= 0);
        bool frameDone = m_queuedTiles == 0;
        bool waitForFrame = m_waitForFrame;
        locker.unlock();

        bool ok = dt->result.get();
        if (!ok) {
            CERR << "error during DecodeTask" << std::endl;
            lock_guard locker(*m_mutex);
            m_status = "decode error";
        }

        auto msg = dt->msg;
        if (frameDone && waitForFrame) {
            finishFrame(*msg);
            assert(!m_waitForFrame);
            return true;
        }

        locker.lock();
    }

    if (m_queuedTiles > 0) {
        return false;
    }

#if 0
    if (!m_queuedTasks.empty() && !(m_queuedTasks.front()->msg->flags & rfbTileFirst)) {
        CERR << "first deferred tile should have rfbTileFirst set" << std::endl;
    }
#endif

    assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
    assert(m_queuedTasks.empty() || m_deferredFrames > 0);

    assert(locker.owns_lock());
    while (canEnqueue()) {
        if (m_queuedTasks.empty())
            break;
        //CERR << "emptying deferred queue: tiles=" << m_queuedTasks.size() << ", frames=" << m_deferredFrames << std::endl;
        auto dt = m_queuedTasks.front();
        m_queuedTasks.pop_front();

        const auto &tile = static_cast<const tileMsg &>(dt->msg->rhr());

        if (tile.flags & rfbTileFirst) {
            --m_deferredFrames;
            assert(m_deferredFrames >= 0);
        }
        if (m_deferredFrames > 0) {
            if (tile.flags & rfbTileFirst) {
                //CERR << "skipping remote frame" << std::endl;
                ++m_remoteSkipped;
                ++m_remoteSkippedPerFrame;
            }
        } else {
            //CERR << "quueing from updateTaskQueue" << std::endl;
            enqueueTask(dt);
        }
    }

    assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
    assert(m_queuedTasks.empty() || m_deferredFrames > 0);

    return false;
}

void RemoteConnection::finishFrame(const RemoteRenderMessage &msg)
{
    {
        std::lock_guard<std::mutex> locker(*m_taskMutex);
        m_frameReady = true;
        m_waitForFrame = false;
        m_newHead = m_receivingHead;
    }

    const auto &tile = static_cast<const tileMsg &>(msg.rhr());
#ifdef CONNDEBUG
    CERR << "finishFrame: #req=" << tile.requestNumber << ", t=" << tile.timestep << ", t req=" << m_requestedTimestep
         << std::endl;
#endif

    m_remoteTimestep = tile.timestep;
    if (m_remoteTimestep >= m_numRemoteTimesteps) {
        CERR << "finishFrame: t=" << tile.timestep << ", but remote claims to only have " << m_numRemoteTimesteps
             << std::endl;
    }
    if (m_requestedTimestep >= 0) {
        if (tile.timestep == m_requestedTimestep) {
        } else if (tile.timestep == m_visibleTimestep) {
            //CERR << "finishFrame: t=" << tile.timestep << ", but req=" << m_requestedTimestep  << " - still showing" << std::endl;
        } else if (tile.timestep == -1 && m_requestedTimestep >= m_numRemoteTimesteps) {
        } else {
            CERR << "finishFrame: t=" << tile.timestep << ", but req=" << m_requestedTimestep << std::endl;
        }
    }

    ++m_remoteFrames;
    double frametime = cover->frameTime();
    double delay = frametime - tile.requestTime;
    m_accumDelay += delay;
    if (m_minDelay > delay)
        m_minDelay = delay;
    if (m_maxDelay < delay)
        m_maxDelay = delay;
    m_avgDelay = 0.7 * m_avgDelay + 0.3 * delay;

    m_depthBytesS += m_depthBytes;
    m_rgbBytesS += m_rgbBytes;
    m_depthBppS = m_depthBpp;
    m_numPixelsS = m_numPixels;
    osg::FrameStamp *frameStamp = VRViewer::instance()->getViewerFrameStamp();
    auto stats = VRViewer::instance()->getViewerStats();
    if (stats && stats->collectStats("frame_rate") && frameStamp) {
        double duration = frametime - m_lastFrameTime;
        stats->setAttribute(frameStamp->getFrameNumber(), "RHR FPS", 1. / duration);
        stats->setAttribute(frameStamp->getFrameNumber(), "RHR Delay", delay);
        stats->setAttribute(frameStamp->getFrameNumber(), "RHR Bps", (m_depthBytes + m_rgbBytes) / duration);
        stats->setAttribute(frameStamp->getFrameNumber(), "RHR Skipped Frames", m_remoteSkippedPerFrame / duration);
    }
    m_lastFrameTime = frametime;
    m_remoteSkippedPerFrame = 0;
}


bool RemoteConnection::checkSwapFrame()
{
#if 0
   static int count = 0;
   CERR << "checkSwapFrame(): " << count << ", frame ready=" << m_frameReady << std::endl;
   ++count;
#endif

    bool doSwap = coVRMSController::instance()->allReduceAnd(m_frameReady);
    return doSwap;
}

void RemoteConnection::swapFrame()
{
    m_head = m_newHead;

    CERR << "swapFrame: timestep=" << m_remoteTimestep << ", " << m_visibleTimestep << " should be visible"
         << std::endl;

    m_drawer->swapFrame();

    std::lock_guard<std::mutex> locker(*m_taskMutex);
    assert(m_frameReady == true);
    m_frameReady = false;
    m_frameDrawn = false;
}

void RemoteConnection::checkDiscardFrame()
{
    auto t = m_numRemoteTimesteps > m_requestedTimestep ? m_requestedTimestep : -1;
    if (m_remoteTimestep != t && m_remoteTimestep != m_visibleTimestep && m_remoteTimestep != m_requestedTimestep) {
        if (checkSwapFrame()) {
            CERR << "discardFrame: timestep=" << m_remoteTimestep << ", " << m_visibleTimestep << " should be visible"
                 << std::endl;

            std::lock_guard<std::mutex> locker(*m_taskMutex);
            assert(m_frameReady == true);
            m_frameReady = false;
            m_frameDrawn = false;

            m_head = m_newHead;
        }
    }
}

void RemoteConnection::processMessages()
{
    unsigned ntiles = 0;
    {
        lock_guard locker(*m_mutex);
        ntiles = m_receivedTiles.size();
        if (!m_lastTileAt.empty()) {
            assert(m_lastTileAt.back() <= m_receivedTiles.size());
            ntiles = m_lastTileAt.front();
        }
        if (coVRMSController::instance()->isCluster())
            ntiles = std::min(ntiles, m_maxTilesPerFrame);
    }
#ifdef CONNDEBUG
    if (ntiles > 0) {
        lock_guard locker(*m_mutex);
        auto &last = m_receivedTiles.back().msg->rhr();
        assert(last.type == rfbTile);
        auto &tile = static_cast<tileMsg &>(last);

        CERR << "have " << ntiles << " image tiles"
             << ", last tile for req: " << tile.requestNumber << ", last=" << (tile.flags & rfbTileLast) << std::endl;
    }
    if (ntiles > 0) {
        std::cerr << "processing " << ntiles << " tiles" << std::endl;
    }
#endif

    if (m_comm) {
        std::shared_ptr<RemoteRenderMessage> msg;
        std::shared_ptr<vistle::buffer> payload;
        if (coVRMSController::instance()->isMaster()) {
            for (unsigned i = 0; i < ntiles; ++i) {
                {
                    lock_guard locker(*m_mutex);
                    TileMessage &tile = m_receivedTiles[i];
                    msg = tile.msg;
                    payload = tile.payload;
                }
                distributeAndHandleTileMpi(msg, payload);
            }
            for (int i = 1; i < m_comm->size(); ++i)
                m_comm->send(i, TagContinue);
        } else {
            while (distributeAndHandleTileMpi(msg, payload))
                ;
        }
    } else {
        const bool broadcastTiles = m_geoMode != RemoteConnection::Screen || m_visibleViews != MultiChannelDrawer::Same;
        const int numSlaves = coVRMSController::instance()->getNumSlaves();
        if (coVRMSController::instance()->isMaster()) {
            // COVER cluster master
            std::vector<int> stiles;
            std::vector<std::vector<bool>> forSlave;
            std::vector<void *> md(ntiles), pd(ntiles);
            std::vector<size_t> ms(ntiles), ps(ntiles);
            {
                lock_guard locker(*m_mutex);
                assert(m_receivedTiles.size() >= ntiles);
                for (unsigned i = 0; i < ntiles; ++i) {
                    TileMessage &tile = m_receivedTiles[i];
                    md[i] = tile.msg.get();
                    ms[i] = sizeof(*tile.msg);
                    ps[i] = tile.msg->payloadSize();
                    pd[i] = nullptr;
                    if (ps[i] > 0) {
                        pd[i] = tile.payload->data();
                        assert(pd[i]);
                    }
                }

                if (!broadcastTiles) {
                    stiles.resize(numSlaves);
                    forSlave.resize(numSlaves);
                    for (int s = 0; s < numSlaves; ++s) {
                        auto &nc = m_nodeConfig[s + 1];
                        int channelBase = nc.viewIndexOffset;
                        const int numChannels = nc.numViews;
                        stiles[s] = 0;
                        forSlave[s].resize(ntiles);
                        for (unsigned i = 0; i < ntiles; ++i) {
                            TileMessage &tile = m_receivedTiles[i];
                            forSlave[s][i] = false;
                            //CERR << "ntiles=" << ntiles << ", have=" << m_receivedTiles.size() << ", i=" << i << std::endl;
                            if (tile.tile.flags) {
                            } else if (m_visibleViews == MultiChannelDrawer::All) {
                            } else if (m_visibleViews == MultiChannelDrawer::Same) {
                                if (tile.tile.viewNum < channelBase || tile.tile.viewNum >= channelBase + numChannels)
                                    continue;
                            } else if (m_visibleViews == MultiChannelDrawer::MatchingEye) {
                                if (tile.tile.eye == rfbEyeMiddle && !nc.haveMiddle)
                                    continue;
                                if (tile.tile.eye == rfbEyeLeft && !nc.haveLeft)
                                    continue;
                                if (tile.tile.eye == rfbEyeRight && !nc.haveRight)
                                    continue;
                            }
                            forSlave[s][i] = true;
                            ++stiles[s];
                        }
                    }
                }
            }
            if (broadcastTiles) {
                //CERR << "broadcasting " << ntiles << " tiles" << std::endl;
                coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
                for (unsigned i = 0; i < ntiles; ++i) {
                    assert(ms[i] == sizeof(RemoteRenderMessage));
                    //CERR << "broadcast tile " << i << std::endl;
                    coVRMSController::instance()->syncData(md[i], ms[i]);
                    if (ps[i] > 0) {
                        coVRMSController::instance()->syncData(pd[i], ps[i]);
                    }
                    lock_guard locker(*m_mutex);
                    TileMessage &tile = m_receivedTiles[i];
                    handleTileMessage(tile.msg, tile.payload);
                }
            } else {
                for (int s = 0; s < numSlaves; ++s) {
                    //CERR << "unicasting " << stiles[s] << " tiles to slave " << s << " << std::endl;
                    coVRMSController::instance()->sendSlave(s, &stiles[s], sizeof(stiles[s]));
                }
                for (unsigned i = 0; i < ntiles; ++i) {
                    for (int s = 0; s < numSlaves; ++s) {
                        if (forSlave[s][i]) {
                            //CERR << "send tile " << i << " to slave " << s << std::endl;
                            coVRMSController::instance()->sendSlave(s, md[i], ms[i]);
                            if (ps[i] > 0) {
                                coVRMSController::instance()->sendSlave(s, pd[i], ps[i]);
                            }
                        }
                    }
                    lock_guard locker(*m_mutex);
                    TileMessage &tile = m_receivedTiles[i];
                    handleTileMessage(tile.msg, tile.payload);
                }
            }
        } else {
            // COVER cluster slave
            if (broadcastTiles) {
                coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
                for (unsigned i = 0; i < ntiles; ++i) {
                    auto msg = std::make_shared<RemoteRenderMessage>(tileMsg());
                    coVRMSController::instance()->syncData(msg.get(), sizeof(*msg));
                    std::shared_ptr<vistle::buffer> payload;
                    if (msg->payloadSize() > 0) {
                        payload.reset(new vistle::buffer(msg->payloadSize()));
                        coVRMSController::instance()->syncData(payload->data(), msg->payloadSize());
                    }
                    lock_guard locker(*m_mutex);
                    handleTileMessage(msg, payload);
                }
            } else {
                coVRMSController::instance()->readMaster(&ntiles, sizeof(ntiles));
                //CERR << "receiving " << ntiles << " tiles" << std::endl;
                for (unsigned i = 0; i < ntiles; ++i) {
                    auto msg = std::make_shared<RemoteRenderMessage>(tileMsg());
                    coVRMSController::instance()->readMaster(msg.get(), sizeof(*msg));
                    std::shared_ptr<vistle::buffer> payload;
                    if (msg->payloadSize() > 0) {
                        payload.reset(new vistle::buffer(msg->payloadSize()));
                        coVRMSController::instance()->readMaster(payload->data(), msg->payloadSize());
                    }
                    lock_guard locker(*m_mutex);
                    handleTileMessage(msg, payload);
                }
            }
        }
    }

    if (coVRMSController::instance()->isMaster()) {
        lock_guard locker(*m_mutex);

        // pop just processed tiles
        for (unsigned i = 0; i < ntiles; ++i) {
            m_receivedTiles.pop_front();
        }
        bool atFrameStart = false;
        while (!m_lastTileAt.empty()) {
            if (ntiles >= m_lastTileAt.front()) {
                m_lastTileAt.pop_front();
                atFrameStart = true;
            } else {
                break;
            }
        }
        for (auto &t: m_lastTileAt)
            t -= ntiles;

        // discard unprocessed frames except for newest
        if (atFrameStart) {
            skipFrames();
        }
    }
}

void RemoteConnection::skipFrames()
{
    if (m_lastTileAt.size() > 1) {
        CERR << "discarding " << m_lastTileAt.size() - 1 << " remote frames" << std::endl;
        m_remoteSkipped += m_lastTileAt.size() - 1;
        m_remoteSkippedPerFrame += m_lastTileAt.size() - 1;
    }
    while (m_lastTileAt.size() > 1) {
        unsigned ntiles = m_lastTileAt.front();
        m_lastTileAt.pop_front();
        for (unsigned i = 0; i < ntiles; ++i)
            m_receivedTiles.pop_front();
        for (auto &t: m_lastTileAt) {
            t -= ntiles;
            assert(t <= m_receivedTiles.size());
        }
    }
}

std::string RemoteConnection::status() const
{
    lock_guard locker(*m_mutex);
    if (m_fps.empty())
        return m_status;
    else
        return m_status + " - " + m_fps;
}

void RemoteConnection::setVisibleTimestep(int t)
{
    lock_guard locker(*m_mutex);
    m_visibleTimestep = t;
}

int RemoteConnection::numTimesteps() const
{
    lock_guard locker(*m_mutex);
    return m_numRemoteTimesteps;
}

int RemoteConnection::currentTimestep() const
{
    lock_guard locker(*m_mutex);
    return std::min(m_remoteTimestep, m_numRemoteTimesteps - 1);
}

osg::Group *RemoteConnection::scene()
{
    return m_scene.get();
}

void RemoteConnection::updateStats(bool print, int localFrames)
{
    double diff = cover->frameTime() - m_lastStat;
    print = print && coVRMSController::instance()->isMaster();

    double depthRate = (double)m_depthBytesS / ((m_remoteFrames)*m_depthBppS * m_numPixelsS);
    double rgbRate = (double)m_rgbBytesS / ((m_remoteFrames)*3 * m_numPixelsS);
    double bandwidthMB = (m_depthBytesS + m_rgbBytesS) / diff / 1024 / 1024;

    if (print) {
        fprintf(stderr, "depth bpp: %ld, depth bytes: %ld, rgb bytes: %ld\n", (long)m_depthBppS, (long)m_depthBytesS,
                (long)m_rgbBytesS);
        if (m_remoteFrames + m_remoteSkipped > 0) {
            fprintf(stderr, "RHR: FPS: local %f, remote %f, SKIPPED: %d, DELAY: min %f, max %f, avg %f\n",
                    localFrames / diff, (m_remoteFrames + m_remoteSkipped) / diff, m_remoteSkipped, m_minDelay,
                    m_maxDelay, m_accumDelay / m_remoteFrames);
        } else {
            fprintf(stderr, "RHR: FPS: local %f, remote %f\n", localFrames / diff,
                    (m_remoteFrames + m_remoteSkipped) / diff);
        }
        fprintf(stderr, "    pixels: %ld, bandwidth: %.2f MB/s", (long)m_numPixelsS, bandwidthMB);
        if (m_remoteFrames > 0)
            fprintf(stderr, ", depth comp: %.1f%%, rgb comp: %.1f%%", depthRate * 100, rgbRate * 100);
        fprintf(stderr, "\n");
    }

    std::stringstream str;
    if (m_remoteFrames + m_remoteSkipped > 0) {
        str << std::fixed << std::setprecision(1) << "F/s: " << (m_remoteFrames + m_remoteSkipped) / diff;
    }
    auto status = str.str();

    m_remoteSkipped = 0;
    m_lastStat = cover->frameTime();
    m_minDelay = DBL_MAX;
    m_maxDelay = 0.;
    m_accumDelay = 0.;
    m_remoteFrames = 0;
    m_depthBytesS = 0;
    m_rgbBytesS = 0;

    lock_guard locker(*m_mutex);
    m_fps = status;
}

const osg::Matrix &RemoteConnection::getHeadMat() const
{
    return m_head;
}

bool RemoteConnection::distributeAndHandleTileMpi(std::shared_ptr<RemoteRenderMessage> msg,
                                                  std::shared_ptr<vistle::buffer> payload)
{
    assert(m_comm);
    if (coVRMSController::instance()->isMaster()) {
        int tag = TagTileAll;
        auto comm = m_comm.get();
        auto &tile = static_cast<tileMsg &>(msg->rhr());
        if (m_geoMode == Screen && !tile.flags) {
            if (m_visibleViews == MultiChannelDrawer::MatchingEye) {
                if (tile.eye == rfbEyeMiddle) {
                    tag = TagTileMiddle;
                    comm = m_commMiddle.get();
                }
                if (tile.eye == rfbEyeLeft) {
                    tag = TagTileLeft;
                    comm = m_commLeft.get();
                }
                if (tile.eye == rfbEyeRight) {
                    tag = TagTileRight;
                    comm = m_commRight.get();
                }
            } else if (m_visibleViews == MultiChannelDrawer::Same) {
                tag = TagTileSend;
            } else {
                tag = TagTileAny;
                comm = m_commAny.get();
            }
        }

        //CERR << "sending with tag " << toString(MpiTags(tag)) << std::endl;

        message::Buffer buf(*msg);
        if (tag == TagTileSend) {
            int dest = 0;
            for (auto &nc: m_nodeConfig) {
                if (dest > 0 && tile.viewNum >= nc.viewIndexOffset && tile.viewNum < nc.viewIndexOffset + nc.numViews) {
                    m_comm->send(dest, TagTileSend, buf.data(), sizeof(RemoteRenderMessage));
                    m_comm->send(dest, TagData, payload->data(), payload->size());
                    break;
                }
                ++dest;
            }
        } else {
            for (int i = 1; i < m_comm->size(); ++i) {
                m_comm->send(i, tag);
            }
            boost::mpi::broadcast(*comm, buf.data(), sizeof(RemoteRenderMessage), 0);
            boost::mpi::broadcast(*comm, payload->data(), payload->size(), 0);
        }

    } else {
        auto status = m_comm->probe();
        //CERR << "probing resulted in tag " << toString(MpiTags(status.tag())) << std::endl;
        if (status.tag() == TagQuit) {
            m_comm->recv(0, TagQuit);
            CERR << "quitting MPI thread" << std::endl;
            return false;
        } else if (status.tag() == TagContinue) {
            m_comm->recv(0, TagContinue);
            return false;
        } else if (status.tag() == TagTileSend) {
            //CERR << "distributeAndHandleTileMpi: send" << std::endl;
            message::Buffer tile;
            m_comm->recv(0, TagTileSend, tile.data(), sizeof(RemoteRenderMessage));
            msg = std::make_shared<RemoteRenderMessage>(tile.as<RemoteRenderMessage>());
            payload = std::make_shared<vistle::buffer>(msg->payloadSize());
            m_comm->recv(0, TagData, payload->data(), msg->payloadSize());
        } else if (status.tag() == TagTileAll || status.tag() == TagTileAny || status.tag() == TagTileMiddle ||
                   status.tag() == TagTileLeft || status.tag() == TagTileRight) {
            m_comm->recv(0, status.tag());
            message::Buffer tile;
            auto *comm = m_comm.get();
            bool participate = true;
            auto &nc = m_nodeConfig[m_comm->rank()];
            if (status.tag() == TagTileAny) {
                comm = m_commAny.get();
                participate = nc.haveMiddle || nc.haveLeft || nc.haveRight;
            }
            if (status.tag() == TagTileMiddle) {
                comm = m_commMiddle.get();
                participate = nc.haveMiddle;
            }
            if (status.tag() == TagTileLeft) {
                comm = m_commLeft.get();
                participate = nc.haveLeft;
            }
            if (status.tag() == TagTileRight) {
                comm = m_commRight.get();
                participate = nc.haveRight;
            }
            //CERR << "distributeAndHandleTileMpi: participate=" << participate << std::endl;
            if (!participate)
                return true;

            boost::mpi::broadcast(*comm, tile.data(), sizeof(RemoteRenderMessage), 0);
            msg = std::make_shared<RemoteRenderMessage>(tile.as<RemoteRenderMessage>());
            payload = std::make_shared<vistle::buffer>(tile.payloadSize());
            boost::mpi::broadcast(*comm, payload->data(), payload->size(), 0);
        }
    }

    assert(msg);
    handleTileMessage(msg, payload);
    return true;
}

void RemoteConnection::setMaxTilesPerFrame(unsigned ntiles)
{
    m_maxTilesPerFrame = ntiles;
}

int RemoteConnection::moduleId() const
{
    return m_moduleId;
}
