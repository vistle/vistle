#include "rhrcontroller.h"
#include <vistle/core/parameter.h>
#include <vistle/core/statetracker.h>
#include <vistle/core/points.h>
#include <vistle/util/hostname.h>
#include <vistle/util/enum.h>
#include <iostream>

#define CERR std::cerr << "RhrController(" << m_module->id() << "): "

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ConnectionMethod,
                                    (ViaVistle)(AutomaticHostname)(UserHostname)(ViaHub)(AutomaticReverse)(UserReverse))

RhrController::RhrController(vistle::Module *module, int displayRank)
: m_module(module)
, m_imageOutPort(nullptr)
, m_displayRank(displayRank)
, m_rhrConnectionMethod(nullptr)
, m_rhrLocalEndpoint(nullptr)
, m_rhrBasePort(nullptr)
, m_forwardPort(0)
, m_rgbaEncoding(nullptr)
, m_rgbaCodec(CompressionParameters::Jpeg_YUV444)
, m_rgbaCompressMode(nullptr)
, m_rgbaCompress(message::CompressionNone)
, m_depthPrec(nullptr)
, m_prec(24)
, m_depthZfpMode(nullptr)
, m_zfpMode(CompressionParameters::ZfpFixedRate)
, m_depthCompressMode(nullptr)
, m_depthCompress(message::CompressionLz4)
, m_sendTileSizeParam(nullptr)
, m_sendTileSize((vistle::Integer)512, (vistle::Integer)512)
{
    m_imageOutPort = m_module->createOutputPort("image_out", "connect to COVER");

    m_rhrConnectionMethod =
        module->addIntParameter("rhr_connection_method", "how local/remote endpoint should be determined",
                                AutomaticHostname, Parameter::Choice);
    module->V_ENUM_SET_CHOICES(m_rhrConnectionMethod, ConnectionMethod);
    m_rhrBasePort = module->addIntParameter("rhr_base_port", "listen port for RHR server", 31590);
    module->setParameterRange(m_rhrBasePort, (Integer)1, (Integer)((1 << 16) - 1));
    m_rhrLocalEndpoint =
        module->addStringParameter("rhr_local_address", "address where clients should connect to", "localhost");
    m_rhrRemotePort = module->addIntParameter("rhr_remote_port", "port where renderer should connect to", 31589);
    module->setParameterRange(m_rhrRemotePort, (Integer)1, (Integer)((1 << 16) - 1));
    m_rhrRemoteEndpoint =
        module->addStringParameter("rhr_remote_host", "address where renderer should connect to", "localhost");
    m_rhrAutoRemotePort = module->addIntParameter("_rhr_auto_remote_port", "port where renderer should connect to", 0);
    module->setParameterReadOnly(m_rhrAutoRemotePort, true);
    module->setParameterRange(m_rhrAutoRemotePort, (Integer)0, (Integer)((1 << 16) - 1));
    m_rhrAutoRemoteEndpoint =
        module->addStringParameter("_rhr_auto_remote_host", "address where renderer should connect to", "localhost");
    module->setParameterReadOnly(m_rhrAutoRemoteEndpoint, true);

    module->setCurrentParameterGroup("Compression");
    m_sendTileSizeParam =
        module->addIntVectorParameter("send_tile_size", "edge lengths of tiles used during sending", m_sendTileSize);
    module->setParameterRange(m_sendTileSizeParam, IntParamVector(1, 1), IntParamVector(65536, 65536));

    std::vector<std::string> choices;
    m_rgbaEncoding = module->addIntParameter("color_codec", "codec for image data", m_rgbaCodec, Parameter::Choice);
    module->V_ENUM_SET_CHOICES_SCOPE(m_rgbaEncoding, CompressionParameters::ColorCodec, CompressionParameters);
    m_rgbaCompressMode =
        module->addIntParameter("color_compress", "compression for RGBA messages", m_rgbaCompress, Parameter::Choice);
    module->V_ENUM_SET_CHOICES(m_rgbaCompressMode, message::CompressionMode);

    m_depthCodecParam =
        module->addIntParameter("depth_codec", "Depth codec", CompressionParameters::DepthRaw, Parameter::Choice);
    module->V_ENUM_SET_CHOICES_SCOPE(m_depthCodecParam, CompressionParameters::DepthCodec, CompressionParameters);
    m_depthZfpMode =
        module->addIntParameter("zfp_mode", "Accuracy:, Precision:, Rate: ", (Integer)m_zfpMode, Parameter::Choice);
    module->V_ENUM_SET_CHOICES_SCOPE(m_depthZfpMode, CompressionParameters::ZfpMode, CompressionParameters);
    m_depthCompressMode = module->addIntParameter("depth_compress", "entropy compression for depth data",
                                                  (Integer)m_depthCompress, Parameter::Choice);
    module->V_ENUM_SET_CHOICES(m_depthCompressMode, message::CompressionMode);

    m_depthPrec =
        module->addIntParameter("depth_prec", "quantized depth precision", (Integer)(m_prec == 24), Parameter::Choice);
    choices.clear();
    choices.emplace_back("16 bit + 4 bits/pixel");
    choices.emplace_back("24 bit + 3 bits/pixel");
    module->setParameterChoices(m_depthPrec, choices);
    module->setCurrentParameterGroup("");

    module->setCurrentParameterGroup("Advanced", false);
    m_dumpImagesParam = module->addIntParameter("rhr_dump_images", "dump image data to disk", (Integer)m_dumpImages,
                                                Parameter::Boolean);
    module->setCurrentParameterGroup("");

    initializeServer();
}

bool RhrController::initializeServer()
{
    if (m_currentClient) {
        m_clientModuleId = m_currentClient->getModuleID();
    } else {
        m_clientModuleId = message::Id::Invalid;
        m_rhr.reset();
        return false;
    }

    if (m_module->rank() != rootRank()) {
        m_rhr.reset();
        return true;
    }

    bool requireServer = true;
    switch (m_rhrConnectionMethod->getValue()) {
    case ViaVistle:
        requireServer = false;
        break;
    case UserReverse:
    case AutomaticReverse:
        requireServer = false;
        break;
    }

    if (m_rhr) {
        // make sure that dtor runs before ctor of new RhrServer
        if (requireServer) {
            if (m_rhr->isConnecting() || m_rhr->port() != m_rhrBasePort->getValue())
                m_rhr.reset();
        } else {
            if (!m_rhr->isConnecting()) {
                CERR << "resetting server: not connecting" << std::endl;
                m_rhr.reset();
            } else if (m_rhrConnectionMethod->getValue() == UserReverse &&
                       (m_rhrRemoteEndpoint->getValue() != m_rhr->destinationHost() ||
                        m_rhrRemotePort->getValue() != m_rhr->destinationPort())) {
                CERR << "resetting server: user reverse, settings mismatch" << std::endl;
                m_rhr.reset();
            } else if (m_rhrConnectionMethod->getValue() == AutomaticReverse &&
                       (m_rhrAutoRemoteEndpoint->getValue() != m_rhr->destinationHost() ||
                        m_rhrAutoRemotePort->getValue() != m_rhr->destinationPort())) {
                CERR << "resetting server: automatic reverse, settings mismatch" << std::endl;
                m_rhr.reset();
            }
        }
    }

    if (!m_rhr) {
        m_rhr.reset(new RhrServer(m_module));
        if (requireServer) {
            m_rhr->startServer(m_rhrBasePort->getValue());
        }
    }

    m_rhr->setDepthPrecision(m_prec);
    m_rhr->setDepthCompression(m_depthCompress);
    m_rhr->setDepthCodec(m_depthCodec);
    m_rhr->setZfpMode(m_zfpMode);
    m_rhr->setColorCodec(m_rgbaCodec);
    m_rhr->setTileSize(m_sendTileSize[0], m_sendTileSize[1]);
    m_rhr->setColorCompression(m_rgbaCompress);

    return true;
}

bool RhrController::handleParam(const vistle::Parameter *p)
{
    if (p == m_rhrBasePort || p == m_rhrRemoteEndpoint || p == m_rhrRemotePort) {
        bool ret = initializeServer();
        sendConfigObject();
        return ret;
    } else if (p == m_rhrConnectionMethod) {
        {
            bool lp = false, lh = false, rp = false, rh = false;
            switch (m_rhrConnectionMethod->getValue()) {
            case ViaVistle:
                lp = rp = lh = rh = true;
                break;
            case AutomaticHostname:
                rp = lh = rh = true;
                break;
            case UserHostname:
                rp = rh = true;
                break;
            case ViaHub:
                rp = lh = rh = true;
                break;
            case AutomaticReverse:
                lp = rh = lh = true;
                break;
            case UserReverse:
                lp = lh = true;
                break;
            }
            m_module->setParameterReadOnly(m_rhrBasePort, lp);
            m_module->setParameterReadOnly(m_rhrLocalEndpoint, lh);
            m_module->setParameterReadOnly(m_rhrRemoteEndpoint, rh);
            m_module->setParameterReadOnly(m_rhrRemotePort, rp);
        }
        if ((m_rhrConnectionMethod->getValue() != ViaHub && m_forwardPort != 0) ||
            m_forwardPort != m_rhrBasePort->getValue()) {
            if (m_module->rank() == 0) {
                if (m_forwardPort)
                    m_module->removePortMapping(m_forwardPort);
            }
            m_forwardPort = 0;
        }
        switch (m_rhrConnectionMethod->getValue()) {
        case ViaVistle: {
            break;
        }
        case AutomaticHostname: {
            break;
        }
        case UserHostname: {
            break;
        }
        case ViaHub: {
            if (m_forwardPort != m_rhrBasePort->getValue()) {
                m_forwardPort = m_rhrBasePort->getValue();
                if (m_module->rank() == 0)
                    m_module->requestPortMapping(m_forwardPort, m_rhrBasePort->getValue());
            }
            break;
        }
        case AutomaticReverse: {
            break;
        }
        case UserReverse: {
            break;
        }
        }
        bool ret = initializeServer();
        sendConfigObject();
        return ret;
    } else if (p == m_depthPrec) {
        if (m_depthPrec->getValue() == 0)
            m_prec = 16;
        else
            m_prec = 24;
        if (m_rhr)
            m_rhr->setDepthPrecision(m_prec);
        return true;
    } else if (p == m_depthCodecParam) {
        m_depthCodec = (CompressionParameters::DepthCodec)m_depthCodecParam->getValue();
        if (m_rhr)
            m_rhr->setDepthCodec(m_depthCodec);
        return true;
    } else if (p == m_depthZfpMode) {
        m_zfpMode = (CompressionParameters::ZfpMode)m_depthZfpMode->getValue();
        if (m_rhr)
            m_rhr->setZfpMode(m_zfpMode);
        return true;
    } else if (p == m_depthCompressMode) {
        m_depthCompress = (message::CompressionMode)m_depthCompressMode->getValue();
        if (m_rhr)
            m_rhr->setDepthCompression(m_depthCompress);
    } else if (p == m_rgbaEncoding) {
        m_rgbaCodec = (CompressionParameters::ColorCodec)m_rgbaEncoding->getValue();
        if (m_rhr)
            m_rhr->setColorCodec(m_rgbaCodec);
        return true;
    } else if (p == m_rgbaCompressMode) {
        m_rgbaCompress = (message::CompressionMode)m_rgbaCompressMode->getValue();
        if (m_rhr)
            m_rhr->setColorCompression(m_rgbaCompress);
    } else if (p == m_sendTileSizeParam) {
        m_sendTileSize = m_sendTileSizeParam->getValue();
        if (m_rhr)
            m_rhr->setTileSize(m_sendTileSize[0], m_sendTileSize[1]);
        return true;
    } else if (p == m_dumpImagesParam) {
        m_dumpImages = m_dumpImagesParam->getValue() != 0;
        if (m_rhr)
            m_rhr->setDumpImages(m_dumpImages);
    }

    return false;
}

Port *RhrController::outputPort() const
{
    return m_imageOutPort;
}

std::shared_ptr<RhrServer> RhrController::server() const
{
    return m_rhr;
}

int RhrController::rootRank() const
{
    return m_displayRank == -1 ? 0 : m_displayRank;
}

bool RhrController::hasConnection() const
{
    if (!m_rhr)
        return false;

    return m_rhr->numClients() > 0;
}

void RhrController::tryConnect(double wait)
{
    if (!m_rhr)
        return;

    if (!outputPort() || !outputPort()->isConnected()) {
        m_rhr->setClientModuleId(message::Id::Invalid);
        return;
    }

    switch (m_rhrConnectionMethod->getValue()) {
    case AutomaticReverse:
    case UserReverse: {
        m_rhr->setClientModuleId(message::Id::Invalid);
        int seconds = wait;
        if (m_rhr->numClients() < 1) {
            std::string host = connectHost();
            unsigned short port = connectPort();
            if (port > 0) {
                m_module->sendInfo("trying for %d seconds to connect to %s:%hu", seconds, host.c_str(), port);
                if (!m_rhr->makeConnection(host, port, seconds)) {
                    m_module->sendWarning("connection attempt to %s:%hu failed", host.c_str(), port);
                }
            } else {
                CERR << "no port specified for connecting to " << host << std::endl;
                return;
            }
        }
        break;
    }
    case ViaVistle: {
        m_rhr->setClientModuleId(m_clientModuleId);
        break;
    }
    default:
        m_rhr->setClientModuleId(message::Id::Invalid);
        return;
    }

    m_rhr->initializeConnection();
}

Object::ptr RhrController::getConfigObject() const
{
    bool valid = true;

    std::stringstream config;
    if (connectionMethod() == RhrController::Connect) {
        auto host = listenHost();
        if (host.empty())
            valid = false;
        unsigned short port = listenPort();
        if (port == 0)
            valid = false;
        config << "connect " << host << " " << port;
    } else if (connectionMethod() == RhrController::Listen) {
        auto host = connectHost();
        unsigned short port = connectPort();
        if (port > 0) {
            config << "listen " << host << " " << port;
        } else {
            int id = m_module->id();
            config << "listen "
                   << "_"
                   << " "
                   << ":" << id;
        }
    } else if (connectionMethod() == RhrController::None) {
        int id = m_module->id();
        config << "vistle module :" << id;
    }

    auto conf = config.str();

    if (!valid) {
        CERR << "not creating config object, invalid: " << conf << std::endl;
        return Points::ptr();
    }

    CERR << "creating config object: " << conf << std::endl;

    Points::ptr points(new Points(size_t(0)));
    points->addAttribute("_rhr_config", conf);
    std::string sender = std::to_string(m_module->id()) + ":" + m_imageOutPort->getName();
    points->addAttribute("_sender", sender);
    points->addAttribute("_plugin", "RhrClient");
    return points;
}

bool RhrController::sendConfigObject() const
{
    if (m_module->rank() == 0) {
        auto obj = getConfigObject();
        if (obj) {
            CERR << "sending rhr config object" << std::endl;
            m_module->updateMeta(obj);
            m_module->addObject(m_imageOutPort, obj);
            return true;
        }
        return false;
    }
    return true;
}

void RhrController::addClient(const Port *client)
{
    if (m_clients.empty()) {
        CERR << "addClient: adding first client" << std::endl;
        m_currentClient = client;

        initializeServer();
        sendConfigObject();
        tryConnect();
    } else {
        CERR << "addClient: already have " << m_clients.size() << " connections, not servicing client port " << *client
             << std::endl;
    }

    m_clients.insert(client);
}

void RhrController::removeClient(const Port *client)
{
    auto it = m_clients.find(client);
    if (it == m_clients.end()) {
        CERR << "removeClient: did not find disconnected destination port " << *client << " among " << m_clients.size()
             << " clients" << std::endl;
        return;
    }

    m_clients.erase(it);
    if (client == m_currentClient) {
        CERR << "removed current client, still have " << m_clients.size() << " clients" << std::endl;
        m_rhr.reset();
        m_currentClient = nullptr;
        m_clientModuleId = message::Id::Invalid;

        if (!m_clients.empty()) {
            m_currentClient = *m_clients.begin();

            initializeServer();
            sendConfigObject();
            tryConnect();
        }
    } else {
        CERR << "removed inactive client, still have " << m_clients.size() << " clients" << std::endl;
        if (m_clients.empty()) {
            m_rhr.reset();
            m_currentClient = nullptr;
            m_clientModuleId = message::Id::Invalid;
        }
    }
}

RhrController::ConnectionDirection RhrController::connectionMethod() const
{
    switch (m_rhrConnectionMethod->getValue()) {
    case ViaVistle:
        return None;
    case AutomaticReverse:
    case UserReverse:
        return Listen;
    default:
        break;
    }
    return Connect;
}

unsigned short RhrController::listenPort() const
{
    if (m_rhr)
        return m_rhr->port();

    return 0;
}

std::string RhrController::listenHost() const
{
    switch (m_rhrConnectionMethod->getValue()) {
    case AutomaticHostname: {
        return hostname();
    }
    case UserHostname: {
        return m_rhrLocalEndpoint->getValue();
    }
    case ViaHub: {
        auto hubdata = m_module->getHub();
        return hubdata.address.to_string();
    }
    case ViaVistle: {
        return "";
    }
    default: {
        break;
    }
    }
    return "localhost";
}

unsigned short RhrController::connectPort() const
{
    switch (m_rhrConnectionMethod->getValue()) {
    case UserReverse:
        return m_rhrRemotePort->getValue();
    case AutomaticReverse:
        return m_rhrAutoRemotePort->getValue();
    default:
        break;
    }

    return 0;
}

std::string RhrController::connectHost() const
{
    switch (m_rhrConnectionMethod->getValue()) {
    case UserReverse:
        return m_rhrRemoteEndpoint->getValue();
    case AutomaticReverse:
        return m_rhrAutoRemoteEndpoint->getValue();
    case ViaVistle: {
        return "";
    }
    default:
        break;
    }

    return "localhost";
}

} // namespace vistle
