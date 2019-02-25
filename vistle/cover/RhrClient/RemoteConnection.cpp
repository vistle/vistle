#include "RemoteConnection.h"
#include "RhrClient.h"

#include <boost/lexical_cast.hpp>

#include <VistlePluginUtil/VistleRenderObject.h>

#include <cover/coVRAnimationManager.h>
#include <cover/coVRMSController.h>
#include <cover/coVRPluginList.h>
#include <cover/coVRPluginSupport.h>
#include <cover/coVRConfig.h>
#include <cover/VRViewer.h>

#include <core/tcpmessage.h>

#include <chrono>

#include "DecodeTask.h"

#ifdef __linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#endif

//#define CONNDEBUG
#define TILE_QUEUE_ON_MAINTHREAD


#define CERR std::cerr << "RemoteConnection: "

namespace asio = boost::asio;

typedef std::lock_guard<std::recursive_mutex> lock_guard;

using namespace opencover;
using namespace vistle;
using message::RemoteRenderMessage;



int RemoteConnection::numViewsForMode(RemoteConnection::GeometryMode mode) {

    switch (mode) {
    case Screen:
        return -1;
    case Tiled:
        return 2;
    case CubeMap:
        return 6;
    case CubeMapFront:
        return 5;
    case Invalid:
        return 0;
    }

    return 0;
}

const osg::BoundingSphere &RemoteConnection::getBounds() const {

    lock_guard locker(*m_mutex);
    return m_boundsNode->getBound();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, std::string host, unsigned short port, bool isMaster)
    : plugin(plugin)
    , m_listen(false)
    , m_host(host)
    , m_port(port)
    , m_sock(plugin->m_io)
    , m_isMaster(isMaster)
{
    init();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, unsigned short port, bool isMaster)
    : plugin(plugin)
    , m_listen(true)
    , m_port(port)
    , m_sock(plugin->m_io)
    , m_isMaster(isMaster)
{
    init();
}

RemoteConnection::RemoteConnection(RhrClient *plugin, unsigned short portFirst, unsigned short portLast, bool isMaster)
    : plugin(plugin)
    , m_listen(true)
    , m_port(0)
    , m_portFirst(portFirst)
    , m_portLast(portLast)
    , m_sock(plugin->m_io)
    , m_isMaster(isMaster)
    , m_setServerParameters(true)
{
    init();
}

RemoteConnection::~RemoteConnection() {

    if (!m_thread) {
        assert(!m_sock.is_open());
    } else {
        stopThread();
        m_thread->join();
        m_thread.reset();
        cover->notify(Notify::Info) << "RhrClient: disconnected from server" << std::endl;
    }
}

void RemoteConnection::init() {

    m_boundsNode = new osg::Node;
    m_mutex.reset(new std::recursive_mutex);
    m_sendMutex.reset(new std::recursive_mutex);
    m_taskMutex.reset(new std::mutex);

    m_drawer = new MultiChannelDrawer(true /* flip top/bottom */);
    m_drawer->setName("RemoteConnectionDrawer");

    m_scene = new osg::Group;
    m_scene->setName("RemoteConnection");

    if (m_isMaster) {
        m_running = true;
        m_thread.reset(new std::thread(std::ref(*this)));
    }
}

void RemoteConnection::setName(const std::string &name) {
    m_name = name;
}

const std::string &RemoteConnection::name() const {
    return m_name;
}

void RemoteConnection::stopThread() {

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
}

void RemoteConnection::operator()() {
#ifdef __linux
    pthread_setname_np(pthread_self(), "RHR:RemoteConnection");
#endif
#ifdef __APPLE__
    pthread_setname_np("RHR:RemoteConnection");
#endif
    {
        lock_guard locker(*m_mutex);
        assert(m_running);
        m_listening = false;
    }

    if (m_listen) {
        boost::system::error_code ec;
        asio::ip::tcp::acceptor acceptor(plugin->m_io);
        int first = m_port, last = m_port;
        if (m_portFirst>0 && m_portLast>0) {
            first = m_portFirst;
            last = m_portLast;
        }
        asio::ip::tcp::endpoint endpoint;
        for (m_port=first; m_port<=last; ++m_port) {
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v6(), m_port);
            acceptor.open(endpoint.protocol(), ec);
            if (ec == boost::system::errc::address_family_not_supported) {
                endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_port);
                acceptor.open(endpoint.protocol(), ec);
            }
            if (ec != boost::system::errc::address_in_use) {
                break;
            }
        }
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: could not open port " << m_port << " for listening: " << ec.message() << std::endl;
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
        acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint, ec);
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: could not bind port " << m_port << ": " << ec.message() << std::endl;
            acceptor.close();
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
        acceptor.listen();
        acceptor.non_blocking(true, ec);
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: could not make acceptor non-blocking: " << ec.message() << std::endl;
            acceptor.close();
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
        {
            lock_guard locker(*m_mutex);
            m_listening = true;
        }
        do {
            acceptor.accept(m_sock, ec);
            if (ec == boost::system::errc::operation_would_block || ec == boost::system::errc::resource_unavailable_try_again) {
                {
                    lock_guard locker(*m_mutex);
                    if (m_interrupt) {
                        break;
                    }
                }
                usleep(1000);
            }
        } while (ec == boost::system::errc::operation_would_block || ec == boost::system::errc::resource_unavailable_try_again);
        acceptor.close();
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: failure to accept client on port " << m_port << ": " << ec.message() << std::endl;
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
    } else {
        asio::ip::tcp::resolver resolver(plugin->m_io);
        asio::ip::tcp::resolver::query query(m_host, boost::lexical_cast<std::string>(m_port));
        boost::system::error_code ec;
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: could not resolve " << m_host << ": " << ec.message() << std::endl;
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
        asio::connect(m_sock, endpoint_iterator, ec);
        if (ec) {
            cover->notify(Notify::Error) << "RhrClient: could not establish connection to " << m_host << ":" << m_port << ": " << ec.message() << std::endl;
            lock_guard locker(*m_mutex);
            m_running = false;
            return;
        }
    }

    {
        lock_guard locker(*m_mutex);
        m_connected = true;
        connectionEstablished();
    }

    for (;;) {
        if (!m_sock.is_open())
        {
            m_connected = false;
            connectionClosed();
            break;
        }

        message::Buffer buf;
        auto payload = std::make_shared<std::vector<char>>();
        bool received = false;
        bool ok = message::recv(m_sock, buf, received, false, payload.get());
        if (!ok) {
            break;
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

        if (buf.type() != message::REMOTERENDERING) {
            CERR << "invalid message type=" << buf.type() << " received" << std::endl;
            break;
        }

        {
            lock_guard locker(*m_mutex);
            haveMessage = true;
            auto &msg = buf.as<RemoteRenderMessage>();
            auto &rhr = msg.rhr();
            switch (rhr.type) {
            case rfbBounds: {
                handleBounds(msg, static_cast<const boundsMsg &>(rhr));
                break;
            }
            case rfbTile: {
                handleTile(msg, static_cast<const tileMsg &>(rhr), payload);
                break;
            }
            case rfbAnimation: {
                handleAnimation(msg, static_cast<const animationMsg &>(rhr));
                break;
            }
            case rfbVariant: {
                handleVariant(msg, static_cast<const variantMsg &>(rhr));
                break;
            }
            }
        }

        lock_guard locker(*m_mutex);
        if (m_interrupt) {
            break;
        }
    }
    lock_guard locker(*m_mutex);
    m_running = false;
}

void RemoteConnection::connectionEstablished() {
    for (const auto &var: m_variantVisibility) {
        variantMsg msg;
        strncpy(msg.name, var.first.c_str(), sizeof(msg.name)-1);
        msg.name[sizeof(msg.name)-1] = '\0';
        msg.visible = var.second;
        send(msg);
    }

    if (m_requestedTimestep >=0)
        requestTimestep(m_requestedTimestep);
    else
        requestTimestep(m_visibleTimestep);
    send(m_lights);
    for (auto &m: m_matrices)
        send(m);

    m_addDrawer = true;
}

void RemoteConnection::connectionClosed() {
}

bool RemoteConnection::requestTimestep(int t, int numTime) {

    lock_guard locker(*m_mutex);

    //CERR << "requesting timestep=" << t << std::endl;
    m_requestedTimestep = t;
    animationMsg msg;
    msg.current = t;
    msg.time = cover->frameTime();
    if (numTime == -1)
        msg.total = coVRAnimationManager::instance()->getNumTimesteps();
    else
        msg.total = numTime;
    return send(msg);
}

bool RemoteConnection::setLights(const lightsMsg &msg) {

    lock_guard locker(*m_mutex);

    if (m_lights != msg) {
        m_lights = msg;
        return send(msg);
    }

    return true;
}

bool RemoteConnection::setMatrices(const std::vector<matricesMsg> &msgs) {

    lock_guard locker(*m_mutex);

    if (m_matrices != msgs) {
        auto dt = cover->frameTime() - m_lastMatricesTime;
        if (dt < std::min(0.3, m_avgDelay*0.5)) {
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

bool RemoteConnection::handleAnimation(const RemoteRenderMessage &msg, const animationMsg &anim) {
    m_numRemoteTimesteps = anim.total;
    CERR << "Animation: #timesteps=" << anim.total << ", cur=" << anim.current << std::endl;
    return true;
}

bool RemoteConnection::handleVariant(const RemoteRenderMessage &msg, const variantMsg &variant) {
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
bool RemoteConnection::update() {

    updateTileQueue();

    processMessages();

    {
        lock_guard locker(*m_mutex);
        coVRMSController::instance()->syncData(&m_connected, sizeof(m_connected));

        updateVariants();
        coVRMSController::instance()->syncData(&m_numRemoteTimesteps, sizeof(m_numRemoteTimesteps));
    }

    updateTileQueue();
    {
        lock_guard locker(*m_mutex);
        coVRMSController::instance()->syncData(&m_remoteTimestep, sizeof(m_remoteTimestep));
    }

    return m_frameReady;
}

void RemoteConnection::preFrame() {

    bool addDrawer = false, connected = false;
    {
        lock_guard locker(*m_mutex);
        coVRMSController::instance()->syncData(&m_addDrawer, sizeof(m_addDrawer));
        addDrawer = m_addDrawer;
        m_addDrawer = false;
        connected = m_connected;
    }

    if (addDrawer) {
        m_scene->addChild(m_drawer);
    } else if (!connected) {
        m_scene->removeChild(m_drawer);
    }

    m_drawer->update();
}

void RemoteConnection::updateVariants() {
    int numAdd = m_variantsToAdd.size(), numRemove = m_variantsToRemove.size();
    coVRMSController::instance()->syncData(&numAdd, sizeof(numAdd));
    coVRMSController::instance()->syncData(&numRemove, sizeof(numRemove));
    auto itAdd = m_variantsToAdd.begin();
    for (int i=0; i<numAdd; ++i) {
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
    for (int i=0; i<numRemove; ++i) {
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

void RemoteConnection::setVariantVisibility(const std::string &variant, bool visible) {
    lock_guard locker(*m_mutex);
    m_variantVisibility[variant] = visible;
}

bool RemoteConnection::handleBounds(const RemoteRenderMessage &msg, const boundsMsg &bound) {

    //CERR << "receiving bounds: center: " << bound.center[0] << " " << bound.center[1] << " " << bound.center[2] << ", radius: " << bound.radius << std::endl;

    m_boundsUpdated = true;
    m_boundsNode->setInitialBound(osg::BoundingSphere(osg::Vec3(bound.center[0], bound.center[1], bound.center[2]), bound.radius));

    //osg::BoundingSphere bs = boundsNode->getBound();
    //CERR << "server bounds: " << bs.center() << ", " << bs.radius() << std::endl;

    return true;
}

bool RemoteConnection::handleTile(const RemoteRenderMessage &msg, const tileMsg &tile, std::shared_ptr<std::vector<char> > payload) {

    assert(msg.rhr().type == rfbTile);
#ifdef CONNDEBUG
    int flags = tile.flags;
    bool first = flags&rfbTileFirst;
    bool last = flags&rfbTileLast;
    CERR << "rfbTileMessage: req: " << tile.requestNumber << ", first: " << first << ", last: " << last << std::endl;
#endif

    if (tile.size==0 && tile.width==0 && tile.height==0 && tile.flags == 0) {
        CERR << "received initial tile - ignoring" << std::endl;
        return true;
    }

    auto m = std::make_shared<RemoteRenderMessage>(msg);
    lock_guard locker(*m_mutex);
    m_receivedTiles.push_back(TileMessage(m, payload));
    if (tile.flags & rfbTileLast)
        m_lastTileAt = m_receivedTiles.size();

    return true;
}

void RemoteConnection::gatherTileStats(const RemoteRenderMessage &remote, const tileMsg &msg) {

  if (msg.flags & rfbTileFirst) {
      m_numPixels = 0;
      m_depthBytes = 0;
      m_rgbBytes = 0;
   }
   if (msg.format != rfbColorRGBA) {
      m_depthBytes += remote.payloadSize();
   } else {
      m_rgbBytes += remote.payloadSize();
      m_numPixels += msg.width*msg.height;
   }
}

void RemoteConnection::handleTileMeta(const RemoteRenderMessage &remote, const tileMsg &msg) {

#ifdef CONNDEBUG
    CERR << "TILE "
         << (msg.flags&rfbTileFirst ? "F" : msg.flags&rfbTileLast ? "L" : ".")
         << " t=" << msg.timestep
         << " req=" << msg.requestNumber << ", view=" << msg.viewNum << ": " << msg.width << "x" << msg.height << "@" << msg.x << "+" << msg.y << std::endl;

   bool first = msg.flags&rfbTileFirst;
   bool last = msg.flags&rfbTileLast;
   if (first || last) {
      CERR << "TILE: " << (first?"F":" ") << "." << (last?"L":" ") << "t=" << msg.timestep << "req: " << msg.requestNumber << ", view: " << msg.viewNum << ", frame: " << msg.frameNumber << ", dt: " << cover->frameTime() - msg.requestTime  << std::endl;
   }
#endif
   osg::Matrix model, view, proj;
   for (int i=0; i<16; ++i) {
      view.ptr()[i] = msg.view[i];
      proj.ptr()[i] = msg.proj[i];
      model.ptr()[i] = msg.model[i];
   }
   int viewIdx = msg.viewNum;
   if (m_geoMode == Screen && !m_renderAllViews) {
       viewIdx -= m_channelBase;
   }
   if (viewIdx < 0 || viewIdx >= m_numViews) {
       CERR << "reject tile with viewIdx=" << viewIdx << ", base=" << m_channelBase << ", numViews=" << m_numViews << std::endl;
       return;
   }
   m_drawer->updateMatrices(viewIdx, model, view, proj);
   int w = msg.totalwidth, h = msg.totalheight;

   if (msg.format == rfbColorRGBA) {
      m_drawer->resizeView(viewIdx, w, h);
   } else {
      GLenum format = GL_FLOAT;
      switch (msg.format) {
         case rfbDepthFloat: format = GL_FLOAT; m_depthBpp=4; break;
         case rfbDepth8Bit: format = GL_UNSIGNED_BYTE; m_depthBpp=1; break;
         case rfbDepth16Bit: format = GL_UNSIGNED_SHORT; m_depthBpp=2; break;
         case rfbDepth24Bit: format = GL_FLOAT; m_depthBpp=4; break;
         case rfbDepth32Bit: format = GL_FLOAT; m_depthBpp=4; break;
         default: CERR << "unhandled image format " << msg.format << std::endl;
      }
      m_drawer->resizeView(viewIdx, w, h, format);
   }
}


bool RemoteConnection::isRunning() const {
    lock_guard locker(*m_mutex);
    return m_running;
}

bool RemoteConnection::isListening() const {
    lock_guard locker(*m_mutex);
    return m_running && m_listening;
}

bool RemoteConnection::isConnecting() const {
    lock_guard locker(*m_mutex);
    return m_running && !m_connected && m_sock.is_open();
}

bool RemoteConnection::isConnected() const {
    lock_guard locker(*m_mutex);
    return m_running && m_connected && m_sock.is_open();
}

bool RemoteConnection::messageReceived() {
    lock_guard locker(*m_mutex);
    bool ret = haveMessage;
    haveMessage = false;
    return ret;
}

bool RemoteConnection::boundsUpdated() {
    lock_guard locker(*m_mutex);
    bool ret = m_boundsUpdated;
    m_boundsUpdated = false;
    return ret;
}

bool RemoteConnection::sendMessage(const message::Message &msg, const std::vector<char> *payload) {
    lock_guard locker(*m_sendMutex);
    if (!isConnected())
        return false;
    if (!message::send(m_sock, msg, payload)) {
        m_sock.close();
        return false;
    }
    return true;
}

bool RemoteConnection::requestBounds() {
    lock_guard locker(*m_mutex);
    if (!isConnected())
        return false;
    m_boundsUpdated = false;

    boundsMsg msg;
    msg.type = rfbBounds;
    msg.sendreply = 1;
    return send(msg);
}

void RemoteConnection::lock() {
    m_mutex->lock();
}

void RemoteConnection::unlock() {
    m_mutex->unlock();
}

void RemoteConnection::setNumLocalViews(int nv) {
    m_numLocalViews = nv;
}

void RemoteConnection::setNumClusterViews(int nv) {
    m_numClusterViews = nv;
}

void RemoteConnection::setNumChannels(const std::vector<int> &numChannels) {
    m_numChannels = numChannels;
}

void RemoteConnection::setFirstView(int v) {
    m_channelBase = v;
}

void RemoteConnection::setGeometryMode(RemoteConnection::GeometryMode mode) {

    if (m_geoMode == mode)
        return;

    m_geoMode = mode;

    m_numViews = numViewsForMode(mode);

    if (mode == Screen) {
        m_drawer->setViewsToRender(MultiChannelDrawer::Same);
        if (m_renderAllViews) {
            m_numViews = m_numClusterViews;
            m_drawer->setNumViews(m_numClusterViews);
        } else {
            m_numViews = m_numLocalViews;
            m_drawer->setNumViews(-1);
        }
    } else if (mode == CubeMap || mode == CubeMapFront || mode == Tiled) {
        m_drawer->setViewsToRender(MultiChannelDrawer::MatchingEye);
        m_drawer->setNumViews(m_numViews);
    } else {
        m_drawer->setNumViews(0);
    }

    CERR << "setGeometryMode(mode=" << mode << "), numViews=" << m_numViews << std::endl;
}

void RemoteConnection::setRenderAllViews(bool state) {

    if (state == m_renderAllViews)
        return;

    m_renderAllViews = state;

    if (m_geoMode != Screen)
        return;

    m_drawer->setViewsToRender(state ? MultiChannelDrawer::MatchingEye : MultiChannelDrawer::Same);
    if (m_renderAllViews) {
        m_numViews = m_numClusterViews;
        m_drawer->setNumViews(m_numClusterViews);
    } else {
        m_numViews = m_numLocalViews;
        m_drawer->setNumViews(-1);
    }

    CERR << "setRenderAllViews(state=" << state << "), numViews=" << m_numViews << std::endl;
}

bool RemoteConnection::canEnqueue() const {

    return !m_frameReady && !m_waitForFrame;
}

void RemoteConnection::enqueueTask(std::shared_ptr<DecodeTask> task) {

   assert(canEnqueue());

   const auto &tile = static_cast<const tileMsg &>(task->msg->rhr());
   handleTileMeta(*task->msg, tile);

   const bool first = tile.flags & rfbTileFirst;
   const bool last = tile.flags & rfbTileLast;
   int view = tile.viewNum;
   if (m_geoMode == RemoteConnection::Screen && !m_renderAllViews) {
       view -= m_channelBase;
   }

   if (view < 0 || view >= m_numViews) {
      //CERR << "NO FB for view " << view << std::endl;
      task->rgba = NULL;
      task->depth = NULL;
   } else {
      task->viewData = m_drawer->getViewData(view);
      task->rgba = reinterpret_cast<char *>(m_drawer->rgba(view));
      task->depth = reinterpret_cast<char *>(m_drawer->depth(view));
      switch (tile.eye) {
      case 0:
          m_drawer->setViewEye(view, Middle);
          break;
      case 1:
          m_drawer->setViewEye(view, Left);
          break;
      case 2:
          m_drawer->setViewEye(view, Right);
          break;
      }
   }
   if (first) {
       assert(m_queuedTiles==0);
   }
   if (last) {
      //CERR << "waiting for frame finish: x=" << task->msg->x << ", y=" << task->msg->y << std::endl;

      m_waitForFrame = true;
   }
   ++m_queuedTiles;

   std::lock_guard<std::mutex> locker(*m_taskMutex);
   m_runningTasks.emplace(task);
   task->result = std::async(std::launch::async, [this, task](){
       bool ok = task->work();
       std::lock_guard<std::mutex> locker(*m_taskMutex);
       m_finishedTasks.emplace_back(task);
       m_runningTasks.erase(task);
       //CERR << "FIN: #running=" << m_runningTasks.size() << ", #fin=" << m_finishedTasks.size() << std::endl;
       return ok;
   });
}

bool RemoteConnection::handleTileMessage(std::shared_ptr<const message::RemoteRenderMessage> msg, std::shared_ptr<std::vector<char>> payload) {

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
      << ", req: " << tile.requestNumber
      << std::endl;
#endif

   if (tile.flags) {
       // meta data for tiles with flags has to be processed on master and all slaves
   } else if (m_geoMode == Screen && !m_renderAllViews) {
       // ignore tiles for other nodes
       if (tile.viewNum < m_channelBase || tile.viewNum >= m_channelBase+m_numLocalViews) {
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
   if (canEnqueue()) {
      enqueueTask(task);
   } else {
      assert(m_deferredFrames >= 0);
      if (tile.flags & rfbTileFirst)
         ++m_deferredFrames;
      std::lock_guard<std::mutex> locker(*m_taskMutex);
      m_queuedTasks.emplace_back(task);
   }

   assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
   assert(m_queuedTasks.empty() || m_deferredFrames>0);

   return true;
}

// returns true if a frame has been discarded - should be called again
bool RemoteConnection::updateTileQueue() {

   //CERR << "tiles: " << m_queuedTiles << " queued, " << m_queuedTasks.size() << " deferred, " << m_finishedTasks.size() << " finished" << std::endl;
   assert(m_queuedTiles >= m_finishedTasks.size());
   while(!m_finishedTasks.empty()) {
      std::shared_ptr<DecodeTask> dt;
      {
          std::lock_guard<std::mutex> locker(*m_taskMutex);
          if (m_finishedTasks.empty())
              break;
          dt = m_finishedTasks.front();
          m_finishedTasks.pop_front();
      }

       bool ok = dt->result.get();
       if (!ok) {
           CERR << "error during DecodeTask" << std::endl;
       }

       --m_queuedTiles;
       assert(m_queuedTiles >= 0);

       auto msg = dt->msg;
       if (m_queuedTiles==0 && m_waitForFrame) {
           finishFrame(*msg);
           assert(!m_waitForFrame);
           return true;
       }
   }

   if (m_queuedTiles > 0) {
      return false;
   }

#if 0
   if (!m_queuedTasks.empty() && !(m_queuedTasks.front()->msg->flags&rfbTileFirst)) {
      CERR << "first deferred tile should have rfbTileFirst set" << std::endl;
   }
#endif

   assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
   assert(m_queuedTasks.empty() || m_deferredFrames>0);

   while(canEnqueue() && !m_queuedTasks.empty()) {
      std::shared_ptr<DecodeTask> dt;
      {
          std::lock_guard<std::mutex> locker(*m_taskMutex);
          if (m_queuedTasks.empty())
              break;
          //CERR << "emptying deferred queue: tiles=" << m_queuedTasks.size() << ", frames=" << m_deferredFrames << std::endl;
          dt = m_queuedTasks.front();
          m_queuedTasks.pop_front();
      }

      const auto &tile = static_cast<const tileMsg &>(dt->msg->rhr());

      if (tile.flags & rfbTileFirst) {
         --m_deferredFrames;
         assert(m_deferredFrames >= 0);
      }
      if (m_deferredFrames > 0) {
          if (tile.flags & rfbTileFirst) {
              //CERR << "skipping remote frame" << std::endl;
              ++m_remoteSkipped;
          }
      } else {
          //CERR << "quueing from updateTaskQueue" << std::endl;
          enqueueTask(dt);
      }
   }

   assert(m_deferredFrames == 0 || !m_queuedTasks.empty());
   assert(m_queuedTasks.empty() || m_deferredFrames>0);

   return false;
}

void RemoteConnection::finishFrame(const RemoteRenderMessage &msg) {

   const auto &tile = static_cast<const tileMsg &>(msg.rhr());
   //CERR << "finishFrame: #req=" << tile.requestNumber << ", t=" << tile.timestep << ", t req=" << m_requestedTimestep << std::endl;

   m_frameReady = true;
   m_waitForFrame = false;

   m_remoteTimestep = tile.timestep;
   if (m_requestedTimestep>=0) {
       if (tile.timestep == m_requestedTimestep) {
       } else if (tile.timestep == m_visibleTimestep) {
           //CERR << "finishFrame: t=" << tile.timestep << ", but req=" << m_requestedTimestep  << " - still showing" << std::endl;
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
   m_avgDelay = 0.7*m_avgDelay + 0.3*delay;

   m_depthBytesS += m_depthBytes;
   m_rgbBytesS += m_rgbBytes;
   m_depthBppS = m_depthBpp;
   m_numPixelsS = m_numPixels;
   osg::FrameStamp *frameStamp = VRViewer::instance()->getViewerFrameStamp();
   auto stats = VRViewer::instance()->getViewerStats();
   if (stats && stats->collectStats("frame_rate") && frameStamp)
   {
       stats->setAttribute(frameStamp->getFrameNumber(), "RHR FPS", 1./(frametime-m_lastFrameTime));
       stats->setAttribute(frameStamp->getFrameNumber(), "RHR Delay", delay);
       stats->setAttribute(frameStamp->getFrameNumber(), "RHR Bps", m_depthBytes+m_rgbBytes);
   }
   m_lastFrameTime = frametime;
}


bool RemoteConnection::checkSwapFrame() {

#if 0
   static int count = 0;
   CERR << "checkSwapFrame(): " << count << ", frame ready=" << m_frameReady << std::endl;
   ++count;
#endif

   bool doSwap = m_frameReady;
   if (coVRMSController::instance()->isMaster()) {
      coVRMSController::SlaveData sd(sizeof(bool));
      coVRMSController::instance()->readSlaves(&sd);
      for (int s=0; s<coVRMSController::instance()->getNumSlaves(); ++s) {
         bool ready = *static_cast<bool *>(sd.data[s]);
         if (!ready) {
            doSwap = false;
            break;
         }
      }
   } else {
      coVRMSController::instance()->sendMaster(&m_frameReady, sizeof(m_frameReady));
   }
   return coVRMSController::instance()->syncBool(doSwap);
}

void RemoteConnection::swapFrame() {

   assert(m_frameReady == true);
   m_frameReady = false;

   //assert(m_visibleTimestep == m_remoteTimestep);
   m_remoteTimestep = -1;

   //CERR << "swapFrame: timestep=" << m_visibleTimestep << std::endl;

   m_drawer->swapFrame();
}

void RemoteConnection::processMessages() {

   int ntiles = 0;
   {
       lock_guard locker(*m_mutex);
       ntiles = m_receivedTiles.size();
       if (m_lastTileAt >= 0) {
           assert(m_lastTileAt <= m_receivedTiles.size());
           ntiles = m_lastTileAt;
           m_lastTileAt = -1;
       } else if (ntiles > 100) {
           ntiles = 100;
       }
   }
#ifdef CONNDEBUG
   if (ntiles > 0) {
       lock_guard locker(*m_mutex);
       auto last = m_receivedTiles.back().msg->rhr();
       assert(last.type == rfbTile);
       auto &tile = static_cast<tileMsg &>(last);

       CERR << "have " << ntiles << " image tiles"
                 << ", last tile for req: " << tile.requestNumber
                 << ", last=" << (tile.flags & rfbTileLast)
                 << std::endl;
   }
#endif

   const bool broadcastTiles = m_geoMode!=RemoteConnection::Screen || m_renderAllViews;
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
           for (int i=0; i<ntiles; ++i) {
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
               int channelBase = coVRConfig::instance()->numChannels();
               for (int s=0; s<numSlaves; ++s) {
                   const int numChannels = m_numChannels[s];
                   stiles[s] = 0;
                   forSlave[s].resize(ntiles);
                   for (int i=0; i<ntiles; ++i) {
                       TileMessage &tile = m_receivedTiles[i];
                       forSlave[s][i] = false;
                       //CERR << "ntiles=" << ntiles << ", have=" << m_receivedTiles.size() << ", i=" << i << std::endl;
                       if (tile.tile.flags) {
                       } else if (tile.tile.viewNum < channelBase || tile.tile.viewNum >= channelBase+numChannels) {
                           continue;
                       }
                       forSlave[s][i] = true;
                       ++stiles[s];
                   }
                   channelBase += numChannels;
               }
           }
       }
       if (broadcastTiles) {
           //CERR << "broadcasting " << ntiles << " tiles" << std::endl;
           coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
           for (int i=0; i<ntiles; ++i) {
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
           for (int s=0; s<numSlaves; ++s) {
               //CERR << "unicasting " << stiles[s] << " tiles to slave " << s << " << std::endl;
               coVRMSController::instance()->sendSlave(s, &stiles[s], sizeof(stiles[s]));
           }
           for (int i=0; i<ntiles; ++i) {
                for (int s=0; s<numSlaves; ++s) {
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

       lock_guard locker(*m_mutex);
       for (int i=0;i<ntiles;++i) {
           m_receivedTiles.pop_front();
       }
       if (m_lastTileAt >= 0)
           m_lastTileAt -= ntiles;
   } else {
       // COVER cluster slave
       if (broadcastTiles) {
           coVRMSController::instance()->syncData(&ntiles, sizeof(ntiles));
           for (int i=0; i<ntiles; ++i) {
               auto msg = std::make_shared<RemoteRenderMessage>(tileMsg());
               coVRMSController::instance()->syncData(msg.get(), sizeof(*msg));
               std::shared_ptr<std::vector<char>> payload;
               if (msg->payloadSize() > 0) {
                   payload.reset(new std::vector<char>(msg->payloadSize()));
                   coVRMSController::instance()->syncData(payload->data(), msg->payloadSize());
               }
               lock_guard locker(*m_mutex);
               handleTileMessage(msg, payload);
           }
       } else {
           coVRMSController::instance()->readMaster(&ntiles, sizeof(ntiles));
           //CERR << "receiving " << ntiles << " tiles" << std::endl;
           for (int i=0; i<ntiles; ++i) {
               auto msg = std::make_shared<RemoteRenderMessage>(tileMsg());
               coVRMSController::instance()->readMaster(msg.get(), sizeof(*msg));
               std::shared_ptr<std::vector<char>> payload;
               if (msg->payloadSize() > 0) {
                   payload.reset(new std::vector<char>(msg->payloadSize()));
                   coVRMSController::instance()->readMaster(payload->data(), msg->payloadSize());
               }
               lock_guard locker(*m_mutex);
               handleTileMessage(msg, payload);
           }
       }
   }
}

MultiChannelDrawer *RemoteConnection::drawer() {
    return m_drawer.get();
}

void RemoteConnection::setVisibleTimestep(int t) {
    lock_guard locker(*m_mutex);
    m_visibleTimestep = t;
}

int RemoteConnection::numTimesteps() const {
    lock_guard locker(*m_mutex);
    return m_numRemoteTimesteps;
}

int RemoteConnection::currentTimestep() const {
    lock_guard locker(*m_mutex);
    if (m_numRemoteTimesteps == 0)
        return -1;
    return m_remoteTimestep;
}

osg::Group *RemoteConnection::scene() {
    return m_scene.get();
}

void RemoteConnection::printStats() {

    double diff = cover->frameTime() - m_lastStat;
    bool print = coVRMSController::instance()->isMaster();

    double depthRate = (double)m_depthBytesS/((m_remoteFrames)*m_depthBppS*m_numPixelsS);
    double rgbRate = (double)m_rgbBytesS/((m_remoteFrames)*3*m_numPixelsS);
    double bandwidthMB = (m_depthBytesS+m_rgbBytesS)/diff/1024/1024;

    if (print) {
        fprintf(stderr, "depth bpp: %ld, depth bytes: %ld, rgb bytes: %ld\n",
                (long)m_depthBppS, (long)m_depthBytesS, (long)m_rgbBytesS);
        if (m_remoteFrames + m_remoteSkipped > 0) {
            fprintf(stderr, "VNC RHR: FPS: local %f, remote %f, SKIPPED: %d, DELAY: min %f, max %f, avg %f\n",
                    m_localFrames/diff, (m_remoteFrames+m_remoteSkipped)/diff,
                    m_remoteSkipped,
                    m_minDelay, m_maxDelay, m_accumDelay/m_remoteFrames);
        } else {
            fprintf(stderr, "VNC RHR: FPS: local %f, remote %f\n",
                    m_localFrames/diff, (m_remoteFrames+m_remoteSkipped)/diff);
        }
        fprintf(stderr, "    pixels: %ld, bandwidth: %.2f MB/s",
                (long)m_numPixelsS, bandwidthMB);
        if (m_remoteFrames > 0)
            fprintf(stderr, ", depth comp: %.1f%%, rgb comp: %.1f%%",
                    depthRate*100, rgbRate*100);
        fprintf(stderr, "\n");
    }

    m_remoteSkipped = 0;
    m_lastStat = cover->frameTime();
    m_minDelay = DBL_MAX;
    m_maxDelay = 0.;
    m_accumDelay = 0.;
    m_localFrames = 0;
    m_remoteFrames = 0;
    m_depthBytesS = 0;
    m_rgbBytesS = 0;
}
