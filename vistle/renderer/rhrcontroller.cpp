#include "rhrcontroller.h"
#include <core/parameter.h>

namespace vistle {

RhrController::RhrController(vistle::Module *module, int displayRank)
: m_module(module)
, m_displayRank(displayRank)
, m_rhrBasePort(nullptr)
, m_rhrForward(nullptr)
, m_forwardPort(0)
, m_rgbaEncoding(nullptr)
, m_rgbaCodec(RhrServer::Jpeg_YUV444)
, m_depthPrec(nullptr)
, m_prec(24)
, m_depthQuant(nullptr)
, m_quant(true)
, m_depthSnappy(nullptr)
, m_snappy(true)
, m_sendTileSizeParam(nullptr)
, m_sendTileSize((vistle::Integer)256, (vistle::Integer)256)
{
   m_rhrBasePort = module->addIntParameter("rhr_base_port", "listen port for RHR server", 31590);
   module->setParameterRange(m_rhrBasePort, (Integer)1, (Integer)((1<<16)-1));
   m_rhrForward = module->addIntParameter("rhr_tunnel", "let client connect through hub", 0, Parameter::Boolean);
   m_sendTileSizeParam = module->addIntVectorParameter("send_tile_size", "edge lengths of tiles used during sending", m_sendTileSize);
   module->setParameterRange(m_sendTileSizeParam, IntParamVector(1,1), IntParamVector(16384, 16384));

   std::vector<std::string> choices;
   m_rgbaEncoding = module->addIntParameter("color_codec", "codec for image data", m_rgbaCodec, Parameter::Choice);
   module->V_ENUM_SET_CHOICES_SCOPE(m_rgbaEncoding, RhrServer::ColorCodec, RhrServer);

   m_depthSnappy = module->addIntParameter("depth_snappy", "compress depth with SNAPPY", (Integer)m_snappy, Parameter::Boolean);
   m_depthQuant = module->addIntParameter("depth_quant", "DXT-like depth quantization", (Integer)m_quant, Parameter::Boolean);

   m_depthPrec = module->addIntParameter("depth_prec", "quantized depth precision", (Integer)(m_prec==24), Parameter::Choice);
   choices.clear();
   choices.push_back("16 bit + 4 bits/pixel");
   choices.push_back("24 bit + 3 bits/pixel");
   module->setParameterChoices(m_depthPrec, choices);

   if (module->rank() == rootRank())
      m_rhr.reset(new RhrServer(m_rhrBasePort->getValue()));
}

bool RhrController::handleParam(const vistle::Parameter *p) {

   if (p == m_rhrBasePort) {

      if (m_module->rank() == rootRank()) {
         if (!m_rhr || m_rhr->port() != m_rhrBasePort->getValue()) {
            m_rhr.reset(); // make sure that dtor runs for ctor of new RhrServer
            m_rhr.reset(new RhrServer(m_rhrBasePort->getValue()));
         }
      } else {
         m_rhr.reset();
      }
      return true;
   } else if (p == m_rhrForward) {
      if (m_rhrForward->getValue()) {
         if (m_forwardPort != m_rhrBasePort->getValue()) {
            if (m_forwardPort) {
               m_module->removePortMapping(m_forwardPort);
            }
            m_forwardPort = m_rhrBasePort->getValue();
            if (m_module->rank() == 0)
               m_module->requestPortMapping(m_forwardPort, m_rhrBasePort->getValue());
         }
      } else {
         if (m_module->rank() == 0) {
            if (m_forwardPort)
               m_module->removePortMapping(m_forwardPort);
         }
         m_forwardPort = 0;
      }
      return true;
   } else if (p == m_depthPrec) {

      if (m_depthPrec->getValue() == 0)
         m_prec = 16;
      else
         m_prec = 24;
      if (m_rhr)
         m_rhr->setDepthPrecision(m_prec);
      return true;
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

std::shared_ptr<RhrServer> RhrController::server() const {
   return m_rhr;
}

int RhrController::rootRank() const {

   return m_displayRank==-1 ? 0 : m_displayRank;
}

}
