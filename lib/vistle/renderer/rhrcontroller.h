#ifndef VISTLE_RENDERER_RHRCONTROLLER_H
#define VISTLE_RENDERER_RHRCONTROLLER_H

#include "renderer.h"
#include <vistle/rhr/rhrserver.h>
#include <boost/asio/ip/address.hpp>
#include <string>

namespace vistle {

class V_RENDEREREXPORT RhrController {
public:
    RhrController(vistle::Module *module, int displayRank);
    bool handleParam(const vistle::Parameter *p);
    Port *outputPort() const;

    std::shared_ptr<RhrServer> server() const;
    int rootRank() const;
    bool hasConnection() const;
    void tryConnect(double wait = 1.);

    void addClient(const Port *client);
    void removeClient(const Port *client);

    enum ConnectionDirection {
        Connect, // client connects to server
        Listen, // server connects to client
        Tunnel, // both server and client connect to rendezvous point
    };

    ConnectionDirection connectionMethod() const; // for the client

    // where this server is listening
    unsigned short listenPort() const;
    std::string listenHost() const;

    // where this server should connect to
    unsigned short connectPort() const;
    std::string connectHost() const;

    void setLinearDepth(bool linear);
    bool linearDepth() const;

private:
    bool initializeServer();
    Object::ptr getConfigObject() const;
    bool sendConfigObject() const;
    std::string tunnelId() const;

    vistle::Module *m_module;
    Port *m_imageOutPort;
    int m_displayRank;

    Port::ConstPortSet m_clients;
    const Port *m_currentClient = nullptr;
    int m_clientModuleId = 0;

    IntParameter *m_rhrConnectionMethod;
    StringParameter *m_rhrLocalEndpoint;
    IntParameter *m_rhrBasePort;
    unsigned short m_forwardPort; //< current port mapping
    StringParameter *m_rhrRemoteEndpoint;
    IntParameter *m_rhrRemotePort;
    StringParameter *m_rhrAutoRemoteEndpoint;
    IntParameter *m_rhrAutoRemotePort;

    IntParameter *m_rgbaEncoding;
    CompressionParameters::ColorCodec m_rgbaCodec;
    IntParameter *m_rgbaCompressMode;
    message::CompressionMode m_rgbaCompress;

    IntParameter *m_depthPrec;
    Integer m_prec;
    IntParameter *m_depthCodecParam = nullptr;
    CompressionParameters::DepthCodec m_depthCodec = CompressionParameters::DepthRaw;
    IntParameter *m_depthZfpMode;
    CompressionParameters::ZfpMode m_zfpMode;
    IntParameter *m_depthCompressMode;
    message::CompressionMode m_depthCompress;
    bool m_linearDepth = false;

    IntVectorParameter *m_sendTileSizeParam;
    IntParamVector m_sendTileSize;

    IntParameter *m_dumpImagesParam = nullptr;
    bool m_dumpImages = false;

    std::shared_ptr<RhrServer> m_rhr;
};

} // namespace vistle
#endif
