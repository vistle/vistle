#ifndef VISTLE_RHRCONTROLLER_H
#define VISTLE_RHRCONTROLLER_H

#include "renderer.h"
#include <rhr/rhrserver.h>
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
   void tryConnect(double wait=1.);

   void addClient(const Port *client);
   void removeClient(const Port *client);

   std::string configString() const;

   enum ConnectionDirection {
       Connect, // client connects to server
       Listen, // server connects to client
   };

   ConnectionDirection connectionMethod() const; // for the client

   // where this server is listening
   unsigned short listenPort() const;
   std::string listenHost() const;

   // where this server should connect to
   unsigned short connectPort() const;
   std::string connectHost() const;

 private:
   bool initializeServer();
   std::string getConfigString() const;
   Object::ptr getConfigObject() const;
   void sendConfigObject() const;

   vistle::Module *m_module;
   Port *m_imageOutPort;
   int m_displayRank;

   Port::ConstPortSet m_clients;

   IntParameter *m_rhrConnectionMethod;
   StringParameter *m_rhrLocalEndpoint;
   IntParameter *m_rhrBasePort;
   unsigned short m_forwardPort; //< current port mapping
   StringParameter *m_rhrRemoteEndpoint;
   IntParameter *m_rhrRemotePort;
   StringParameter *m_rhrAutoRemoteEndpoint;
   IntParameter *m_rhrAutoRemotePort;

   IntParameter *m_rgbaEncoding;
   RhrServer::ColorCodec m_rgbaCodec;
   IntParameter *m_rgbaCompressMode;
   message::CompressionMode m_rgbaCompress;

   IntParameter *m_depthPrec;
   Integer m_prec;
   IntParameter *m_depthQuant;
   bool m_quant;
   IntParameter *m_depthZfp;
   bool m_zfp;
   IntParameter *m_depthZfpMode;
   RhrServer::ZfpMode m_zfpMode;
   IntParameter *m_depthCompressMode;
   message::CompressionMode m_depthCompress;

   IntVectorParameter *m_sendTileSizeParam;
   IntParamVector m_sendTileSize;

   std::shared_ptr<RhrServer> m_rhr;
};

}
#endif

