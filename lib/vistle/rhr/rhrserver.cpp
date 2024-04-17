/**\file
 * \brief RhrServer plugin class
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2012, 2013, 2014, 2015, 2016, 2017 HLRS
 *
 * \copyright GPL2+
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>

#include "rfbext.h"
#include "depthquant.h"
#include "rhrserver.h"
#include "compdecomp.h"

#include <vistle/util/stopwatch.h>
#include <vistle/util/netpbmimage.h>
#include <vistle/util/crypto.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/core/messages.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/threadname.h>
#include <vistle/module/module.h>


#define CERR std::cerr << "RHR: "

namespace vistle {

bool RhrServer::send(const RemoteRenderMessage &msg, const buffer *payload)
{
    message::Buffer buf(msg);
    auto &rem = buf.as<RemoteRenderMessage>();
    rem.rhr().modificationCount = m_modificationCount;
    return send(buf, payload);
}

bool RhrServer::send(message::Buffer msg, const buffer *payload)
{
    if (m_clientModuleId != message::Id::Invalid) {
        msg.setDestId(m_clientModuleId);
        return m_module->sendMessage(msg, payload);
    }

    if (m_clientSocket && !m_clientSocket->is_open()) {
        resetClient();
        CERR << "client disconnected" << std::endl;
    }

    if (!m_clientSocket) {
        return false;
    }

    message::error_code ec;
    if (!message::send(*m_clientSocket, msg, ec, payload)) {
        if (ec) {
            CERR << "client error: " << ec.message() << ", disconnecting" << std::endl;
        } else {
            CERR << "unknown client error, disconnecting" << std::endl;
        }
        resetClient();
        return false;
    }
    return true;
}

//! called when plugin is loaded
RhrServer::RhrServer(vistle::Module *module)
: m_module(module), m_acceptorv4(m_io), m_acceptorv6(m_io), m_listen(true), m_port(0), m_destPort(0)
{
    init();
}

// this is called if the plugin is removed at runtime
RhrServer::~RhrServer()
{
    while (m_queuedTiles > 0) {
        usleep(10000);
        std::lock_guard<std::mutex> locker(m_taskMutex);
        m_queuedTiles -= m_finishedTasks.size();
        m_finishedTasks.clear();
    }
    m_clientSocket.reset();

    //fprintf(stderr,"RhrServer::~RhrServer\n");
}

int RhrServer::numClients() const
{
    if (m_clientSocket)
        return 1;

    return 0;
}

bool RhrServer::isConnecting() const
{
    return !m_listen;
}

bool RhrServer::isConnected() const
{
    if (m_clientModuleId != message::Id::Invalid)
        return true;

    return m_clientSocket.get();
}

void RhrServer::setColorCodec(CompressionParameters::ColorCodec value)
{
    m_imageParam.rgbaParam.rgbaCodec = value;
}

void RhrServer::setDepthCodec(CompressionParameters::DepthCodec value)
{
    m_imageParam.depthParam.depthCodec = value;
}

void RhrServer::setColorCompression(message::CompressionMode mode)
{
    m_imageParam.rgbaParam.rgbaCompress = mode;
}

void RhrServer::setDepthCompression(message::CompressionMode mode)
{
    m_imageParam.depthParam.depthCompress = mode;
}

void RhrServer::setDepthPrecision(int bits)
{
    m_imageParam.depthParam.depthPrecision = bits;
}

void RhrServer::setLinearDepth(bool linear)
{
    m_imageParam.depthParam.depthGL = !linear;
}

void RhrServer::setZfpMode(CompressionParameters::ZfpMode mode)
{
    m_imageParam.depthParam.depthZfpMode = mode;
}

void RhrServer::setDumpImages(bool enable)
{
    m_dumpImages = enable;
}

void RhrServer::setClientModuleId(int moduleId)
{
    m_clientModuleId = moduleId;
}

unsigned short RhrServer::port() const
{
    return m_port;
}

unsigned short RhrServer::destinationPort() const
{
    return m_destPort;
}

const std::string &RhrServer::destinationHost() const
{
    return m_destHost;
}


size_t RhrServer::numViews() const
{
    return m_viewData.size();
}

unsigned char *RhrServer::rgba(size_t i)
{
    if (i > numViews())
        return nullptr;
    return m_viewData[i].rgba.data();
}

const unsigned char *RhrServer::rgba(size_t i) const
{
    if (i > numViews())
        return nullptr;
    return m_viewData[i].rgba.data();
}

float *RhrServer::depth(size_t i)
{
    if (i > numViews())
        return nullptr;
    return m_viewData[i].depth.data();
}

const float *RhrServer::depth(size_t i) const
{
    if (i > numViews())
        return nullptr;
    return m_viewData[i].depth.data();
}

int RhrServer::width(size_t i) const
{
    if (i > numViews())
        return 0;
    return m_viewData[i].param.width;
}

int RhrServer::height(size_t i) const
{
    if (i > numViews())
        return 0;
    return m_viewData[i].param.height;
}

const vistle::Matrix4 &RhrServer::viewMat(size_t i) const
{
    return m_viewData[i].param.view;
}

const vistle::Matrix4 &RhrServer::projMat(size_t i) const
{
    return m_viewData[i].param.proj;
}

const vistle::Matrix4 &RhrServer::modelMat(size_t i) const
{
    return m_viewData[i].param.model;
}

void RhrServer::setBoundingSphere(const vistle::Vector3 &center, const vistle::Scalar &radius)
{
    //std::cerr << "RhrServer: new bounding sphere: center=" << center << ", r=" << radius << std::endl;

    m_boundCenter = center;
    m_boundRadius = radius;
}

void RhrServer::updateVariants(
    const std::vector<std::pair<std::string, vistle::RenderObject::InitialVariantVisibility>> &added,
    const std::vector<std::string> &removed)
{
    for (const auto &var: removed) {
        auto it = m_localVariants.find(var);
        if (it != m_localVariants.end())
            m_localVariants.erase(it);
        if (isConnected()) {
            variantMsg msg;
            strncpy(msg.name, var.c_str(), sizeof(msg.name));
            msg.remove = 1;
            send(msg);
        }
    }

    for (const auto &var: added) {
        auto it = m_localVariants.find(var.first);
        if (it == m_localVariants.end()) {
            m_localVariants.emplace(var.first, var.second);
        }
        if (isConnected()) {
            variantMsg msg;
            strncpy(msg.name, var.first.c_str(), sizeof(msg.name));
            if (var.second != vistle::RenderObject::DontChange) {
                msg.configureVisibility = 1;
                msg.visible = var.second == vistle::RenderObject::Visible ? 1 : 0;
            }
            send(msg);
        }
    }
}

//! called after plug-in is loaded and scenegraph is initialized
void RhrServer::init()
{
    if (!crypto::initialize(sizeof(message::Identify::session_data_t))) {
        CERR << "failed to initialize cryptographic support" << std::endl;
        throw except::exception("failed to initialize cryptographic support");
    }

    lightsUpdateCount = 0;

    m_tileWidth = 256;
    m_tileHeight = 256;

    m_numTimesteps = 0;

    m_boundCenter = vistle::Vector3(0., 0., 0.);
    m_boundRadius = 1.;

    m_imageParam.rgbaParam.rgbaCompress = message::CompressionLz4;

    m_imageParam.depthParam.depthPrecision = 32;
    m_imageParam.depthParam.depthCodec = CompressionParameters::DepthRaw;
    m_imageParam.depthParam.depthCompress = message::CompressionLz4;
    m_imageParam.depthParam.depthFloat = true;
    m_imageParam.depthParam.depthGL = true;
    m_imageParam.depthParam.depthZfpMode = CompressionParameters::ZfpFixedRate;

    m_resizeBlocked = false;
    m_queuedTiles = 0;
    m_firstTile = false;

    resetClient();
}

void RhrServer::resetClient()
{
    finishTiles(RhrServer::ViewParameters(), true /* finish */, false /* send */);
    joinWorkerThreads();

    ++m_updateCount;
    ++lightsUpdateCount;
    if (m_clientSocket)
        m_clientSocket->close();
    m_clientSocket.reset();
    lightsUpdateCount = 0;
    m_clientVariants.clear();
    m_viewData.clear();
    m_clientModuleId = message::Id::Invalid;
}

bool RhrServer::startServer(unsigned short port)
{
    m_listen = true;

    boost::system::error_code ec;
    while (!start_listen(port, m_acceptorv4, m_acceptorv6, ec)) {
        if (ec == boost::system::errc::address_in_use) {
            ++port;
        } else {
            CERR << "listening on port " << port << " failed: " << ec.message() << std::endl;
            return false;
        }
    }

    m_port = port;
    CERR << "listening for connections on port " << port << std::endl;

    bool ret = true;
    if (m_acceptorv4.is_open())
        ret &= startAccept(m_acceptorv4);
    if (m_acceptorv6.is_open())
        ret &= startAccept(m_acceptorv6);
    return ret;
}

bool RhrServer::startAccept(asio::ip::tcp::acceptor &a)
{
    auto sock = std::make_shared<asio::ip::tcp::socket>(m_io);

    a.async_accept(*sock, [this, &a, sock](boost::system::error_code ec) { handleAccept(a, sock, ec); });
    return true;
}

void RhrServer::handleAccept(asio::ip::tcp::acceptor &a, std::shared_ptr<asio::ip::tcp::socket> sock,
                             const boost::system::error_code &error)
{
    if (error) {
        CERR << "error in accept: " << error.message() << std::endl;
        return;
    }

    if (m_clientSocket) {
#if 0
       CERR << "incoming connection, rejecting as already servicing a client" << std::endl;
       startAccept();
       return;
#else
        CERR << "incoming connection, disconnecting from current client" << std::endl;
        resetClient();
#endif
    }

    CERR << "incoming connection, accepting new client" << std::endl;
    m_clientSocket = sock;

    send(message::Identify());
    initializeConnection();

    startAccept(a);
}

bool RhrServer::makeConnection(const std::string &host, unsigned short port, int secondsToTry,
                               const std::string &tunnelId)
{
    m_listen = false;

    if (tunnelId.empty()) {
        CERR << "connecting to " << host << ":" << port << "..." << std::endl;
    } else {
        CERR << "connecting tunnel via " << host << ":" << port << "..." << std::endl;
    }

    asio::ip::tcp::resolver resolver(m_io);
    asio::ip::tcp::resolver::query query(host, std::to_string(port), asio::ip::tcp::resolver::query::numeric_service);
    boost::system::error_code ec;
    asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ec);
    if (ec) {
        CERR << "could not resolve " << host << ": " << ec.message() << std::endl;
        return false;
    }

    std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_io));
    int i = 0;
    while (secondsToTry <= 0 || i < secondsToTry * 10) {
        asio::connect(*sock, endpoint_iterator, ec);
        if (secondsToTry == 0)
            break;
        if (ec != boost::system::errc::connection_refused) {
            break;
        }
        if (i % 10 == 0 && i > 0) {
            CERR << "still trying to connect after " << i / 10 << " seconds" << std::endl;
        }
        usleep(100000);
        ++i;
    }

    if (ec) {
        CERR << "could not connect to " << host << ":" << port << ": " << ec.message() << std::endl;
        return false;
    }

    m_clientSocket = sock;

    m_destHost = host;
    m_destPort = port;
    m_tunnelId = tunnelId;

    if (tunnelId.empty()) {
        CERR << "connected to " << host << ":" << port << std::endl;
        m_tunnelEstablished = true;
    } else {
        CERR << "connected to tunnel at " << host << ":" << port << std::endl;
        m_tunnelEstablished = false;
    }
    send(message::Identify());

    return true;
}

bool RhrServer::initializeConnection()
{
    animationMsg anim;
    anim.current = m_imageParam.timestep;
    anim.total = m_numTimesteps;
    return send(anim);
}

void RhrServer::resize(size_t viewNum, int w, int h)
{
#if 0
    if (w!=-1 && h!=-1) {
        std::cout << "resize: view=" << viewNum << ", w=" << w << ", h=" << h << std::endl;
    }
#endif
    while (viewNum >= numViews()) {
        m_viewData.emplace_back();
    }

    ViewData &vd = m_viewData[viewNum];
    if (m_resizeBlocked) {
        if (w != -1 && h != -1) {
            vd.newWidth = w;
            vd.newHeight = h;
        }
        return;
    }

    if (w == -1 && h == -1) {
        w = vd.newWidth;
        h = vd.newHeight;
    }

    if (w == -1 || h == -1) {
        //CERR << "rejecting resize for view " << viewNum << " to " << w << "x" << h << std::endl;
        return;
    }

    if (vd.nparam.width != w || vd.nparam.height != h) {
        vd.nparam.width = w;
        vd.nparam.height = h;

        CERR << "resizing view " << viewNum << " to " << w << "x" << h << std::endl;

        w = std::max(1, w);
        h = std::max(1, h);

        vd.rgba.resize(w * h * 4);
        vd.depth.resize(w * h);
    }
}

void RhrServer::deferredResize()
{
    assert(!m_resizeBlocked);
    for (size_t i = 0; i < numViews(); ++i) {
        resize(i, -1, -1);
    }
}

//! handle matrix update message
bool RhrServer::handleMatrices(std::shared_ptr<socket> sock, const matricesMsg &mat)
{
    if (mat.viewNum < 0) {
        CERR << "invalid view no. " << mat.viewNum << std::endl;
        return false;
    }

    size_t viewNum = mat.viewNum >= 0 ? mat.viewNum : 0;
    if (viewNum >= m_viewData.size()) {
        m_viewData.resize(viewNum + 1);
    }

    resize(viewNum, mat.width, mat.height);

    ViewData &vd = m_viewData[viewNum];

    vd.nparam.timestep = timestep();
    vd.nparam.matrixTime = mat.time;
    vd.nparam.requestNumber = mat.requestNumber;
    vd.nparam.eye = mat.eye;

    for (int i = 0; i < 16; ++i) {
        vd.nparam.head.data()[i] = mat.head[i];
        vd.nparam.proj.data()[i] = mat.proj[i];
        vd.nparam.view.data()[i] = mat.view[i];
        vd.nparam.model.data()[i] = mat.model[i];
    }

    //std::cerr << "handleMatrices: view " << mat.viewNum << ", proj: " << vd.nparam.proj << std::endl;

    if (mat.last) {
        m_viewData.resize(viewNum + 1);
        for (size_t i = 0; i < numViews(); ++i) {
            m_viewData[i].param = m_viewData[i].nparam;
        }
    }

    return true;
}

//! handle light update message
bool RhrServer::handleLights(std::shared_ptr<socket> sock, const lightsMsg &light)
{
#define SET_VEC(d, dst, src) \
    do { \
        for (int k = 0; k < d; ++k) { \
            (dst)[k] = (src)[k]; \
        } \
    } while (false)

    std::vector<Light> newLights;
    for (int i = 0; i < lightsMsg::NumLights; ++i) {
        const auto &cl = light.lights[i];
        newLights.emplace_back();
        auto &l = newLights.back();

        SET_VEC(4, l.position, cl.position);
        SET_VEC(4, l.ambient, cl.ambient);
        SET_VEC(4, l.diffuse, cl.diffuse);
        SET_VEC(4, l.specular, cl.specular);
        SET_VEC(3, l.attenuation, cl.attenuation);
        SET_VEC(3, l.direction, cl.spot_direction);
        l.spotCutoff = cl.spot_cutoff;
        l.spotExponent = cl.spot_exponent;
        l.enabled = cl.enabled;
        //std::cerr << "Light " << i << ": ambient: " << l.ambient << std::endl;
        //std::cerr << "Light " << i << ": diffuse: " << l.diffuse << std::endl;
    }

    std::swap(lights, newLights);
    if (lights != newLights) {
        ++lightsUpdateCount;
        std::cerr << "handleLightsMessage: lights changed" << std::endl;
    }

    //std::cerr << "handleLightsMessage: " << lights.size() << " lights received" << std::endl;

    return true;
}

int RhrServer::timestep() const
{
    return m_imageParam.timestep;
}

void RhrServer::setNumTimesteps(unsigned num)
{
    if (num != m_numTimesteps) {
        m_numTimesteps = num;

        if (isConnected()) {
            animationMsg anim;
            anim.current = m_imageParam.timestep;
            anim.total = num;
            send(anim);
        }
    }
}

size_t RhrServer::updateCount() const
{
    return m_updateCount;
}

const RhrServer::VariantVisibilityMap &RhrServer::getVariants() const
{
    return m_clientVariants;
}

//! send bounding sphere of scene to a client
void RhrServer::sendBoundsMessage(std::shared_ptr<socket> sock)
{
#if 0
   std::cerr << "sending bounds: "
             << "c: " << m_boundCenter
             << "r: " << m_boundRadius << std::endl;
#endif

    boundsMsg msg;
    msg.type = rfbBounds;
    msg.center[0] = m_boundCenter[0];
    msg.center[1] = m_boundCenter[1];
    msg.center[2] = m_boundCenter[2];
    msg.radius = m_boundRadius;

    send(msg);
}


//! handle request for a bounding sphere update
bool RhrServer::handleBounds(std::shared_ptr<socket> sock, const boundsMsg &bound)
{
    if (bound.sendreply) {
        //std::cout << "SENDING BOUNDS" << std::endl;
        sendBoundsMessage(sock);
    }

    return true;
}

bool RhrServer::handleAnimation(std::shared_ptr<RhrServer::socket> sock, const animationMsg &anim)
{
    //CERR << "app timestep: " << anim.current << std::endl;
    m_imageParam.timestep = anim.current;
    m_imageParam.timestepRequestTime = anim.time;

    return true;
}

bool RhrServer::handleVariant(std::shared_ptr<RhrServer::socket> sock, const variantMsg &variant)
{
    CERR << "app variant: " << variant.name << ", visible: " << variant.visible << std::endl;
    std::string name(variant.name);
    bool visible = variant.visible;
    m_clientVariants[name] = visible;
    ++m_updateCount;
    return true;
}

//! this is called before every frame, used for polling for RFB messages
void RhrServer::preFrame()
{
    if (m_clientModuleId != vistle::message::Id::Invalid)
        return;

    m_io.poll();
    if (!isConnected())
        return;

    bool received = false;
    do {
        received = false;
        message::Buffer msg;
        buffer payload;
        MessagePayload payloadShm;
        message::error_code ec;
        if (m_clientSocket) {
            size_t avail = 0;
            asio::socket_base::bytes_readable command(true);
            m_clientSocket->io_control(command);
            if (!m_clientSocket->is_open()) {
                resetClient();
                break;
            }
            if (command.get() > 0) {
                avail = command.get();
            }
            if (avail < sizeof(message::RemoteRenderMessage))
                continue;

            received = message::recv(*m_clientSocket, msg, ec, false, &payload);
            if (!received) {
                if (ec) {
                    CERR << "error during message receive: " << ec.message() << std::endl;
                }
                continue;
            }
        } else {
            CERR << "no possibility for input" << std::endl;
            break;
        }

        switch (msg.type()) {
        case message::TUNNELESTABLISHED: {
            auto &m = msg.as<message::TunnelEstablished>();
            CERR << "tunnel established: " << m << std::endl;
            m_tunnelEstablished = true;
            if (m.role() == message::TunnelEstablished::Server) {
                send(message::Identify());
            }
            break;
        }
        case message::IDENTIFY: {
            auto &m = msg.as<message::Identify>();
            CERR << "client identity: " << m << std::endl;
            using message::Identify;
            switch (m.identity()) {
            case Identify::REQUEST: {
                if (m_tunnelEstablished) {
                    Identify id(m, Identify::RENDERSERVER);
                    send(id);
                } else {
                    send(message::Identify(m, m_tunnelId, Identify::Server));
                }
                break;
            }
            case Identify::RENDERCLIENT: {
                if (!m.verifyMac()) {
                    CERR << "MAC verification failed" << std::endl;
                    resetClient();
                    break;
                }
                for (auto &var: m_localVariants) {
                    variantMsg msg;
                    strncpy(msg.name, var.first.c_str(), sizeof(msg.name));
                    msg.visible = var.second;
                    send(msg);
                }
                break;
            }
            case Identify::HUB: {
                break;
            }
            default: {
                CERR << "unexpected client identity: " << m << std::endl;
                resetClient();
                break;
            }
            }
            break;
        }
        case message::REMOTERENDERING: {
            auto &m = msg.as<message::RemoteRenderMessage>();
            handleRemoteRenderMessage(m_clientSocket, m);
            break;
        }
        default: {
            CERR << "invalid message type " << toString(msg.type()) << " received" << std::endl;
            break;
        }
        }
    } while (received);
}

void RhrServer::invalidate(int viewNum, int x, int y, int w, int h, const RhrServer::ViewParameters &param,
                           bool lastView)
{
    if (isConnected())
        encodeAndSend(viewNum, x, y, w, h, param, lastView);
}

void RhrServer::updateModificationCount()
{
    ++m_modificationCount;
}

namespace {

tileMsg *newTileMsg(const RhrServer::ImageParameters &param, const RhrServer::ViewParameters &vp, int viewNum, int x,
                    int y, int w, int h)
{
    assert(x + w <= std::max(0, vp.width));
    assert(y + h <= std::max(0, vp.height));

    tileMsg *message = new tileMsg;

    message->viewNum = viewNum;
    message->eye = vp.eye;
    message->width = w;
    message->height = h;
    message->x = x;
    message->y = y;
    message->totalwidth = vp.width;
    message->totalheight = vp.height;
    message->size = 0;
    message->compression = rfbTileRaw;
    message->unzippedsize = 0;

    message->frameNumber = vp.frameNumber;
    message->requestNumber = vp.requestNumber;
    message->requestTime = vp.matrixTime > param.timestepRequestTime ? vp.matrixTime : param.timestepRequestTime;
    message->timestep = vp.timestep;
    //std::cerr << "new tile msg: timestep=" << vp.timestep << std::endl;
    for (int i = 0; i < 16; ++i) {
        message->head[i] = vp.head.data()[i];
        message->model[i] = vp.model.data()[i];
        message->view[i] = vp.view.data()[i];
        message->proj[i] = vp.proj.data()[i];
    }

    return message;
}

} // namespace

struct EncodeTask {
    float *depth = nullptr;
    unsigned char *rgba = nullptr;
    tileMsg *message = nullptr;
    const RhrServer::ImageParameters &param;
    int viewNum;
    int x, y, w, h, stride;
    int bpp;
    bool subsamp;
    RhrServer::EncodeResult result;

    EncodeTask() = delete;
    EncodeTask(const EncodeTask &other) = delete;
    EncodeTask(const EncodeTask &&other) = delete;
    EncodeTask &operator=(const EncodeTask &other) = delete;

    EncodeTask(int viewNum, int x, int y, int w, int h, float *depth, const RhrServer::ImageParameters &param,
               const RhrServer::ViewParameters &vp)
    : depth(depth), param(param), viewNum(viewNum), x(x), y(y), w(w), h(h), stride(vp.width), bpp(4), subsamp(false)
    {
        assert(depth);
        message = newTileMsg(param, vp, viewNum, x, y, w, h);

        message->size = message->width * message->height;
        if (param.depthParam.depthFloat) {
            if (param.depthParam.depthGL)
                message->format = rfbDepthFloat;
            else
                message->format = rfbDepthViewer;
            bpp = 4;
        } else {
            switch (param.depthParam.depthPrecision) {
            case 8:
                message->format = rfbDepth8Bit;
                bpp = 1;
                break;
            case 16:
                message->format = rfbDepth16Bit;
                bpp = 2;
                break;
            case 24:
                message->format = rfbDepth24Bit;
                bpp = 4;
                break;
            case 32:
                message->format = rfbDepth32Bit;
                bpp = 4;
                break;
            }
        }

        message->size = bpp * message->width * message->height;

        if (param.depthParam.depthFloat) {
            if (param.depthParam.depthGL)
                message->format = rfbDepthFloat;
            else
                message->format = rfbDepthViewer;
        }

#ifdef HAVE_ZFP
        if (param.depthParam.depthCodec == CompressionParameters::DepthZfp) {
            message->compression |= rfbTileDepthZfp;
        } else
#endif
            if (param.depthParam.depthCodec == CompressionParameters::DepthQuant) {
            message->format = param.depthParam.depthPrecision <= 16 ? rfbDepth16Bit : rfbDepth24Bit;
            message->compression |= rfbTileDepthQuantize;
        } else if (param.depthParam.depthCodec == CompressionParameters::DepthQuantPlanar) {
            message->format = param.depthParam.depthPrecision <= 16 ? rfbDepth16Bit : rfbDepth24Bit;
            message->compression |= rfbTileDepthQuantizePlanar;
        } else if (param.depthParam.depthCodec == CompressionParameters::DepthPredict) {
            message->compression |= rfbTileDepthPredict;
        } else if (param.depthParam.depthCodec == CompressionParameters::DepthPredictPlanar) {
            message->compression |= rfbTileDepthPredictPlanar;
        }
    }

    EncodeTask(int viewNum, int x, int y, int w, int h, unsigned char *rgba, const RhrServer::ImageParameters &param,
               const RhrServer::ViewParameters &vp)
    : rgba(rgba)
    , param(param)
    , viewNum(viewNum)
    , x(x)
    , y(y)
    , w(w)
    , h(h)
    , stride(vp.width)
    , bpp(4)
    , subsamp(param.rgbaParam.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411)
    {
        assert(rgba);
        message = newTileMsg(param, vp, viewNum, x, y, w, h);
        message->size = message->width * message->height * bpp;
        message->format = rfbColorRGBA;

        if (param.rgbaParam.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV411 ||
            param.rgbaParam.rgbaCodec == vistle::CompressionParameters::Jpeg_YUV444) {
            message->compression |= rfbTileJpeg;
        } else if (param.rgbaParam.rgbaCodec == vistle::CompressionParameters::PredictRGB) {
            message->compression |= rfbTilePredictRGB;
        } else if (param.rgbaParam.rgbaCodec == vistle::CompressionParameters::PredictRGBA) {
            message->compression |= rfbTilePredictRGBA;
        }
    }

    RhrServer::EncodeResult work()
    {
        auto &msg = *message;
        RhrServer::EncodeResult result(message);
        message::CompressionMode compress = message::CompressionNone;
        if (depth) {
            compress = param.depthParam.depthCompress;
            auto p = param.depthParam;
            result.payload = compressDepth(depth, x, y, w, h, stride, p);
        } else if (rgba) {
            compress = param.rgbaParam.rgbaCompress;
            auto p = param.rgbaParam;
            result.payload = compressRgba(rgba, x, y, w, h, stride, p);
        }

        msg.unzippedsize = msg.size = result.payload.size();
        result.rhrMessage = std::make_unique<RemoteRenderMessage>(msg, result.payload.size());
        result.payload = message::compressPayload(compress, *result.rhrMessage, result.payload);
        msg.size = result.payload.size();

        return result;
    }
};

void RhrServer::setTileSize(int w, int h)
{
    m_tileWidth = w;
    m_tileHeight = h;
}

void RhrServer::encodeAndSend(int viewNum, int x0, int y0, int w, int h, const RhrServer::ViewParameters &param,
                              bool lastView)
{
    //std::cerr << "encodeAndSend: view=" << viewNum << ", c=" << (void *)rgba(viewNum) << ", d=" << depth(viewNum) << std::endl;
    if (!m_resizeBlocked) {
        m_firstTile = true;
    }
    m_resizeBlocked = true;

    if (m_dumpImages && viewNum >= 0 && size_t(viewNum) < m_viewData.size()) {
        std::stringstream cn, dn, dnfull;
        cn << "color_frame" << m_framecount << "_view" << viewNum << ".ppm";
        dn << "depth_frame" << m_framecount << "_view" << viewNum << ".pgm";
        dnfull << "depth_frame" << m_framecount << "_view" << viewNum << "_full.pgm";
        auto &vp = m_viewData[viewNum].param;
        NetpbmImage dfull(dnfull.str(), vp.width, vp.height, NetpbmImage::PGM, (1U << 24) - 1);
        NetpbmImage d(dn.str(), vp.width, vp.height, NetpbmImage::PGM);
        NetpbmImage c(cn.str(), vp.width, vp.height, NetpbmImage::PPM);

        float dmin = std::numeric_limits<float>::max(), dmax = std::numeric_limits<float>::lowest();
        size_t numpix = vp.width * vp.height;
        for (size_t i = 0; i < numpix; ++i) {
            auto *col = &rgba(viewNum)[i * 4];
            c.append(col[0] / 255.f, col[1] / 255.f, col[2] / 255.f);

            auto dval = depth(viewNum)[i];
            dmin = std::min(dmin, dval);
            dmax = std::max(dmax, dval);

            d.append(dval);
            dfull.append(dval);
        }

        std::cerr << "Dumped depth to " << dn.str() << ", min=" << dmin << ", max=" << dmax << ": " << dfull
                  << std::endl;
    }

    //vistle::StopWatch timer("encodeAndSend");
    const int tileWidth = m_tileWidth, tileHeight = m_tileHeight;

    if (viewNum >= 0) {
        for (int y = y0; y < y0 + h; y += tileHeight) {
            for (int x = x0; x < x0 + w; x += tileWidth) {
                // depth
                auto dt =
                    std::make_shared<EncodeTask>(viewNum, x, y, std::min(tileWidth, x0 + w - x),
                                                 std::min(tileHeight, y0 + h - y), depth(viewNum), m_imageParam, param);

                // color
                auto ct =
                    std::make_shared<EncodeTask>(viewNum, x, y, std::min(tileWidth, x0 + w - x),
                                                 std::min(tileHeight, y0 + h - y), rgba(viewNum), m_imageParam, param);

                std::unique_lock locker(m_taskMutex);
                ++m_queuedTiles;
                m_queuedTasks.emplace_back(dt);
                ++m_queuedTiles;
                m_queuedTasks.emplace_back(ct);
            }
        }
    }

    joinWorkerThreads();

    std::unique_lock locker(m_taskMutex);
    while (m_workers.size() < m_doneWorkers.size() + std::thread::hardware_concurrency() && !m_queuedTasks.empty()) {
        locker.unlock();

        auto thr = std::thread([this]() {
            setThreadName("RHR:Encode");
            std::unique_lock locker(m_taskMutex);
            while (!m_queuedTasks.empty()) {
                auto task = m_queuedTasks.front();
                m_queuedTasks.pop_front();
                locker.unlock();

                task->result = task->work();

                locker.lock();
                m_finishedTasks.emplace_back(task);
            }
            m_doneWorkers.emplace(std::this_thread::get_id());
            locker.unlock();
        });
        m_workers.emplace(thr.get_id(), std::move(thr));

        locker.lock();
    }
    locker.unlock();

    finishTiles(param, lastView || viewNum < 0);
    joinWorkerThreads();
}

bool RhrServer::finishTiles(const RhrServer::ViewParameters &param, bool finish, bool sendTiles)
{
    bool tileReady = false;
    do {
        buffer payload;
        tileReady = false;
        std::unique_ptr<RemoteRenderMessage> msg;
        if (m_queuedTiles == 0 && finish) {
            auto *tm = newTileMsg(m_imageParam, param, -1, 0, 0, 0, 0);
            msg = std::make_unique<RemoteRenderMessage>(*tm);
            delete tm;
        } else {
            std::lock_guard<std::mutex> locker(m_taskMutex);
            if (!m_finishedTasks.empty()) {
                auto &task = m_finishedTasks.front();
                auto &result = task->result;
                msg = std::move(result.rhrMessage);
                payload = std::move(result.payload);
                m_finishedTasks.pop_front();
                --m_queuedTiles;
                tileReady = true;
            }
        }
        if (msg) {
            tileMsg &tm = static_cast<tileMsg &>(msg->rhr());
            tm.timestep = param.timestep;
            if (m_firstTile) {
                tm.flags |= rfbTileFirst;
                //std::cerr << "first tile: req=" << msg.requestNumber << std::endl;
            }
            m_firstTile = false;
            if (m_queuedTiles == 0 && finish) {
                tm.flags |= rfbTileLast;
                //std::cerr << "last tile: req=" << msg.requestNumber << std::endl;
            }
            tm.frameNumber = m_framecount;
            if (sendTiles)
                send(*msg, &payload);
        }
        if (m_queuedTiles > 0 && finish && !tileReady)
            usleep(100);
    } while (m_queuedTiles > 0 && (tileReady || finish));

    if (finish) {
        assert(m_queuedTiles == 0);
        m_resizeBlocked = false;
        deferredResize();

        ++m_framecount;
    }

    return m_queuedTiles == 0;
}

bool RhrServer::joinWorkerThreads()
{
    std::unique_lock locker(m_taskMutex);
    for (auto id: m_doneWorkers) {
        auto it = m_workers.find(id);
        if (it != m_workers.end()) {
            auto &thr = it->second;
            thr.join();
            m_workers.erase(it);
        }
    }
    m_doneWorkers.clear();
    return m_workers.empty();
}

RhrServer::ViewParameters RhrServer::getViewParameters(int viewNum) const
{
    return m_viewData[viewNum].param;
}

bool RhrServer::handleMessage(const message::Message *message, const MessagePayload &payload)
{
    if (message->type() != message::REMOTERENDERING)
        return false;

    auto rr = message->as<message::RemoteRenderMessage>();

    return handleRemoteRenderMessage(nullptr, rr);
}

bool RhrServer::handleRemoteRenderMessage(std::shared_ptr<socket> sock, const vistle::message::RemoteRenderMessage &rr)
{
    const auto &rhr = rr.rhr();
    switch (rhr.type) {
    case rfbMatrices: {
        const auto &mat = static_cast<const matricesMsg &>(rhr);
        handleMatrices(sock, mat);
        break;
    }
    case rfbLights: {
        const auto &light = static_cast<const lightsMsg &>(rhr);
        handleLights(sock, light);
        break;
    }
    case rfbAnimation: {
        const auto &anim = static_cast<const animationMsg &>(rhr);
        handleAnimation(sock, anim);
        break;
    }
    case rfbBounds: {
        const auto &bound = static_cast<const boundsMsg &>(rhr);
        handleBounds(sock, bound);
        break;
    }
    case rfbVariant: {
        const auto &var = static_cast<const variantMsg &>(rhr);
        handleVariant(sock, var);
        break;
    }
    case rfbTile:
    default:
        CERR << "invalid RHR message subtype " << rhr.type << " received" << std::endl;
        return false;
    }
    return true;
}

} // namespace vistle
