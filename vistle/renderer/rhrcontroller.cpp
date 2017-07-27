#include "rhrcontroller.h"
#include <core/parameter.h>
#include <core/statetracker.h>
#include <core/points.h>
#include <util/hostname.h>

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ConnectionMethod, (AutomaticHostname)(UserHostname)(ViaHub)(Reverse))

RhrController::RhrController(vistle::Module *module, int displayRank)
: m_module(module)
, m_imageOutPort(nullptr)
, m_displayRank(displayRank)
, m_rhrConnectionMethod(nullptr)
, m_rhrLocalEndpoint(nullptr)
, m_rhrBasePort(nullptr)
, m_forwardPort(0)
, m_rgbaEncoding(nullptr)
, m_rgbaCodec(RhrServer::Jpeg_YUV444)
, m_depthPrec(nullptr)
, m_prec(24)
, m_depthQuant(nullptr)
, m_quant(true)
, m_depthSnappy(nullptr)
, m_snappy(true)
, m_depthZfp(nullptr)
, m_zfp(false)
, m_depthZfpMode(nullptr)
, m_zfpMode(RhrServer::ZfpFixedRate)
, m_sendTileSizeParam(nullptr)
, m_sendTileSize((vistle::Integer)256, (vistle::Integer)256)
{
   m_imageOutPort = m_module->createOutputPort("image_out", "connect to COVER");

   m_rhrConnectionMethod = module->addIntParameter("rhr_connection_method", "how local/remote endpoint should be determined", AutomaticHostname, Parameter::Choice);
   module->V_ENUM_SET_CHOICES(m_rhrConnectionMethod, ConnectionMethod);
   m_rhrBasePort = module->addIntParameter("rhr_base_port", "listen port for RHR server", 31590);
   module->setParameterRange(m_rhrBasePort, (Integer)1, (Integer)((1<<16)-1));
   m_rhrLocalEndpoint = module->addStringParameter("rhr_local_address", "address where clients should connect to", "localhost");
   m_rhrRemotePort = module->addIntParameter("rhr_remote_port", "port where renderer should connect to", 31589);
   module->setParameterRange(m_rhrRemotePort, (Integer)1, (Integer)((1<<16)-1));
   m_rhrRemoteEndpoint = module->addStringParameter("rhr_remote_host", "address where renderer should connect to", "localhost");

   m_sendTileSizeParam = module->addIntVectorParameter("send_tile_size", "edge lengths of tiles used during sending", m_sendTileSize);
   module->setParameterRange(m_sendTileSizeParam, IntParamVector(1,1), IntParamVector(16384, 16384));

   std::vector<std::string> choices;
   m_rgbaEncoding = module->addIntParameter("color_codec", "codec for image data", m_rgbaCodec, Parameter::Choice);
   module->V_ENUM_SET_CHOICES_SCOPE(m_rgbaEncoding, RhrServer::ColorCodec, RhrServer);

   m_depthZfp = module->addIntParameter("depth_zfp", "compress depth with zfp floating point compressor", (Integer)m_zfp, Parameter::Boolean);
   m_depthZfpMode = module->addIntParameter("zfp_mode", "Accuracy:, Precision:, Rate: ", (Integer)m_zfpMode, Parameter::Choice);
   module->V_ENUM_SET_CHOICES_SCOPE(m_depthZfpMode, RhrServer::ZfpMode, RhrServer);
   m_depthSnappy = module->addIntParameter("depth_snappy", "compress depth with SNAPPY", (Integer)m_snappy, Parameter::Boolean);
   m_depthQuant = module->addIntParameter("depth_quant", "DXT-like depth quantization", (Integer)m_quant, Parameter::Boolean);

   m_depthPrec = module->addIntParameter("depth_prec", "quantized depth precision", (Integer)(m_prec==24), Parameter::Choice);
   choices.clear();
   choices.push_back("16 bit + 4 bits/pixel");
   choices.push_back("24 bit + 3 bits/pixel");
   module->setParameterChoices(m_depthPrec, choices);

   initializeServer();
}

bool RhrController::initializeServer() {

   if (m_module->rank() != rootRank()) {
       m_rhr.reset();
       return true;
   }

   // make sure that dtor runs before ctor of new RhrServer
   if (m_rhr && m_rhrConnectionMethod->getValue()==Reverse) {
       if (!m_rhr->isConnecting() || m_rhrRemoteEndpoint->getValue() != m_rhr->destinationHost() || m_rhrRemotePort ->getValue() != m_rhr->destinationPort())
           m_rhr.reset();
   }

   if (m_rhr && m_rhrConnectionMethod->getValue()!=Reverse) {
       if (m_rhr->isConnecting() || m_rhr->port() != m_rhrBasePort->getValue())
           m_rhr.reset();
   }

   if (!m_rhr) {
       m_rhr.reset(new RhrServer());
       if (m_rhrConnectionMethod->getValue() != Reverse) {
           m_rhr->startServer(m_rhrBasePort->getValue());
       }
   }

   m_rhr->setDepthPrecision(m_prec);
   m_rhr->enableDepthZfp(m_zfp);
   m_rhr->setZfpMode(m_zfpMode);
   m_rhr->enableDepthSnappy(m_snappy);
   m_rhr->enableQuantization(m_quant);
   m_rhr->setColorCodec(m_rgbaCodec);
   m_rhr->setTileSize(m_sendTileSize[0], m_sendTileSize[1]);

   return true;
}

bool RhrController::handleParam(const vistle::Parameter *p) {

   if (p == m_rhrBasePort || p == m_rhrRemoteEndpoint || p == m_rhrRemotePort) {

      return initializeServer();
   } else if (p == m_rhrConnectionMethod) {

      if ((m_rhrConnectionMethod->getValue() != ViaHub && m_forwardPort != 0) || m_forwardPort != m_rhrBasePort->getValue()) {
          if (m_module->rank() == 0) {
            if (m_forwardPort)
               m_module->removePortMapping(m_forwardPort);
         }
         m_forwardPort = 0;
      }
      switch (m_rhrConnectionMethod->getValue()) {
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
      case Reverse: {
          break;
      }
      }
      return initializeServer();
   } else if (p == m_depthPrec) {

      if (m_depthPrec->getValue() == 0)
         m_prec = 16;
      else
         m_prec = 24;
      if (m_rhr)
         m_rhr->setDepthPrecision(m_prec);
      return true;
   } else if (p == m_depthZfp) {

      m_zfp = m_depthZfp->getValue();
      if (m_rhr)
         m_rhr->enableDepthZfp(m_zfp);
      return true;
   } else if (p == m_depthZfpMode) {
       m_zfpMode = (RhrServer::ZfpMode)m_depthZfpMode->getValue();
       if (m_rhr)
           m_rhr->setZfpMode(m_zfpMode);
   } else if (p == m_depthSnappy) {

      m_snappy = m_depthSnappy->getValue();
      if (m_rhr)
         m_rhr->enableDepthSnappy(m_snappy);
      return true;
   } else if (p == m_depthQuant) {

      m_quant = m_depthQuant->getValue();
      if (m_rhr)
         m_rhr->enableQuantization(m_quant);
      return true;
   } else if (p == m_rgbaEncoding) {

      m_rgbaCodec = (RhrServer::ColorCodec)m_rgbaEncoding->getValue();
      if (m_rhr)
         m_rhr->setColorCodec(m_rgbaCodec);
      return true;
   } else if (p == m_sendTileSizeParam) {

      m_sendTileSize = m_sendTileSizeParam->getValue();
      if (m_rhr)
         m_rhr->setTileSize(m_sendTileSize[0], m_sendTileSize[1]);
      return true;
   }

   return false;
}

Port *RhrController::outputPort() const {

    return m_imageOutPort;
}

std::shared_ptr<RhrServer> RhrController::server() const {
   return m_rhr;
}

int RhrController::rootRank() const {

    return m_displayRank==-1 ? 0 : m_displayRank;
}

void RhrController::tryConnect() {

    if (!m_rhr)
        return;

    switch(m_rhrConnectionMethod->getValue()) {
    case Reverse:
        if (m_rhr->numClients() < 1) {
            std::string host = m_rhrRemoteEndpoint->getValue();
            unsigned short port = m_rhrRemotePort->getValue();
            m_module->sendInfo("trying to connect to %s:%hu", host.c_str(), port);
            if (!m_rhr->makeConnection(m_rhrRemoteEndpoint->getValue(), m_rhrRemotePort->getValue(), 10)) {
                m_module->sendWarning("connection to %s:%hu failed", host.c_str(), port);
            }
        }
        break;
    default:
        return;
    }
}

std::string RhrController::getConfigString() const {

    std::stringstream config;
    if (connectionMethod() == RhrController::Connect) {
        auto host = listenHost();
        unsigned short port = listenPort();
        config << "connect " << host << " " << port;
    } else {
        auto host = connectHost();
        unsigned short port = connectPort();
        config << "listen " << host << " " << port;
    }

    return config.str();
}

Object::ptr RhrController::getConfigObject() const {

    auto conf = getConfigString();
    std::cerr << "ParallelRemoteRenderManager: creating config object: " << conf << std::endl;

    Points::ptr points(new Points(Index(0)));
    points->addAttribute("_rhr_config", conf);
    points->addAttribute("_plugin", "RhrClient");
    return points;
}

void RhrController::sendConfigObject() const {

    if (m_module->rank() == 0) {
        std::cerr << "sending rhr config object" << std::endl;
        static_cast<Module *>(m_module)->addObject(m_imageOutPort, getConfigObject());
    }
}

void RhrController::addClient(const Port *client) {

    if (!m_clients.empty()) {
        std::cerr << "ParallelRemoteRenderManager: not servicing client port " << *client << std::endl;
    }

    m_clients.insert(client);

    if (!m_rhr) {
        initializeServer();
    }
    sendConfigObject();
    tryConnect();
}

void RhrController::removeClient(const Port *client) {

    auto it = m_clients.find(client);
    if (it == m_clients.end()) {
        std::cerr << "ParallelRemoteRenderManager: did not find disconnected destination port " << *client << std::endl;
        return;
    }

    m_clients.erase(it);

    if (m_clients.empty()) {
        m_rhr.reset();
    }

    if (m_module->rank() == 0) {
        std::cerr << "sending rhr config object" << std::endl;
        static_cast<Module *>(m_module)->addObject(m_imageOutPort, getConfigObject());
    }
}

RhrController::ConnectionDirection RhrController::connectionMethod() const {

    switch(m_rhrConnectionMethod->getValue()) {
    case Reverse:
        return Listen;
    default:
        break;
    }
    return Connect;
}

unsigned short RhrController::listenPort() const {

    if (m_rhr)
        return m_rhr->port();

    return 0;
}

std::string RhrController::listenHost() const {

    switch(m_rhrConnectionMethod->getValue()) {
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
    default: {

        break;
    }
    }
    return "localhost";
}

unsigned short RhrController::connectPort() const {

    return m_rhrRemotePort->getValue();
}

std::string RhrController::connectHost() const {

    switch (m_rhrConnectionMethod->getValue()) {
    case Reverse:
        return m_rhrRemoteEndpoint->getValue();
    default:
        break;
    }

    return "localhost";
}

}
