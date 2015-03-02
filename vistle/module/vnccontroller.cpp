#include "vnccontroller.h"
#include <core/parameter.h>

namespace vistle {

VncController::VncController(vistle::Module *module, int displayRank)
: m_module(module)
, m_displayRank(displayRank)
, m_vncBasePort(nullptr)
, m_vncForward(nullptr)
, m_forwardPort(0)
, m_vncReverse(nullptr)
, m_vncRevPort(nullptr)
, m_rgbaEncoding(nullptr)
, m_rgbaCodec(VncServer::Jpeg)
, m_depthPrec(nullptr)
, m_prec(24)
, m_depthQuant(nullptr)
, m_quant(true)
, m_depthSnappy(nullptr)
, m_snappy(true)
, m_sendTileSizeParam(nullptr)
, m_sendTileSize((vistle::Integer)256, (vistle::Integer)256)
{
   m_vncBasePort = module->addIntParameter("vnc_base_port", "listen port for VNC server", 31590);
   m_vncOnSlaves = module->addIntParameter("vnc_on_slaves", "start VNC servers on slave ranks", 0, Parameter::Boolean);
   module->setParameterRange(m_vncBasePort, (Integer)1, (Integer)((1<<16)-1));
   m_vncReverse = module->addIntParameter("vnc_reverse", "establish VNC connection from server to client", 0, Parameter::Boolean);
   m_vncRevHost = module->addStringParameter("vnc_reverse_host", "establish reverse connection to client on host", "localhost");
   m_vncRevPort = module->addIntParameter("vnc_reverse_port", "establish reverse connection to client on port", 31059);
   m_vncForward = module->addIntParameter("vnc_tunnel", "let client connect through hub", 0, Parameter::Boolean);
   module->setParameterRange(m_vncRevPort, (Integer)1, (Integer)((1<<16)-1));
   m_sendTileSizeParam = module->addIntVectorParameter("send_tile_size", "edge lengths of tiles used during sending", m_sendTileSize);
   module->setParameterRange(m_sendTileSizeParam, IntParamVector(1,1), IntParamVector(16384, 16384));

   std::vector<std::string> choices;
   m_rgbaEncoding = module->addIntParameter("color_codec", "codec for image data", m_rgbaCodec, Parameter::Choice);
   choices.clear();
   choices.push_back("Raw");
   choices.push_back("JPEG");
   choices.push_back("SNAPPY");
   module->setParameterChoices(m_rgbaEncoding, choices);

   m_depthSnappy = module->addIntParameter("depth_snappy", "compress depth with SNAPPY", (Integer)m_snappy, Parameter::Boolean);
   m_depthQuant = module->addIntParameter("depth_quant", "DXT-like depth quantization", (Integer)m_quant, Parameter::Boolean);

   m_depthPrec = module->addIntParameter("depth_prec", "quantized depth precision", (Integer)(m_prec==24), Parameter::Choice);
   choices.clear();
   choices.push_back("16 bit + 4 bits/pixel");
   choices.push_back("24 bit + 3 bits/pixel");
   module->setParameterChoices(m_depthPrec, choices);

   if (module->rank() == rootRank() || m_vncOnSlaves->getValue())
      m_vnc.reset(new VncServer(1024, 768, m_vncBasePort->getValue()));
}

bool VncController::handleParam(const vistle::Parameter *p) {

   if (p == m_vncOnSlaves) {

      if (m_module->rank() != rootRank()) {

         if (m_vncOnSlaves->getValue()) {
            if (!m_vnc) {
               m_vnc.reset(new VncServer(1024, 768, m_vncBasePort->getValue()));
            }
         } else {
            m_vnc.reset();
         }
      }
      return true;
   } else if (p == m_vncBasePort
         || p == m_vncReverse
         || p == m_vncRevHost
         || p == m_vncRevPort) {

      if (m_vncReverse->getValue()) {
         if (m_module->rank() == rootRank()) {
            if (!m_vnc || m_vnc->port() != m_vncRevPort->getValue() || m_vnc->host() != m_vncRevHost->getValue()) {
               m_vnc.reset(); // make sure that dtor runs for ctor of new VncServer
               m_vnc.reset(new VncServer(1024, 768, m_vncRevHost->getValue(), m_vncRevPort->getValue()));
            }
         } else {
            m_vnc.reset();
         }
      } else {
         if (m_module->rank() == rootRank() || m_vncOnSlaves->getValue()) {
            if (!m_vnc || m_vnc->port() != m_vncBasePort->getValue()) {
               m_vnc.reset(); // make sure that dtor runs for ctor of new VncServer
               m_vnc.reset(new VncServer(1024, 768, m_vncBasePort->getValue()));
            }
         } else {
            m_vnc.reset();
         }
      }
      return true;
   } else if (p == m_vncForward) {
      if (m_vncForward->getValue()) {
         if (m_forwardPort != m_vncBasePort->getValue()) {
            if (m_forwardPort) {
               m_module->removePortMapping(m_forwardPort);
            }
            m_forwardPort = m_vncBasePort->getValue();
            if (m_module->rank() == 0)
               m_module->requestPortMapping(m_forwardPort, m_vncBasePort->getValue());
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
      if (m_vnc)
         m_vnc->setDepthPrecision(m_prec);
      return true;
   } else if (p == m_depthSnappy) {

      m_snappy = m_depthSnappy->getValue();
      if (m_vnc)
         m_vnc->enableDepthSnappy(m_snappy);
      return true;
   } else if (p == m_depthQuant) {

      m_quant = m_depthQuant->getValue();
      if (m_vnc)
         m_vnc->enableQuantization(m_quant);
      return true;
   } else if (p == m_rgbaEncoding) {

      m_rgbaCodec = (VncServer::ColorCodec)m_rgbaEncoding->getValue();
      if (m_vnc)
         m_vnc->setColorCodec(m_rgbaCodec);
      return true;
   } else if (p == m_sendTileSizeParam) {

      m_sendTileSize = m_sendTileSizeParam->getValue();
      if (m_vnc)
         m_vnc->setTileSize(m_sendTileSize[0], m_sendTileSize[1]);
      return true;
   }

   return false;
}

boost::shared_ptr<VncServer> VncController::server() const {
   return m_vnc;
}

int VncController::rootRank() const {

   return m_displayRank==-1 ? 0 : m_displayRank;
}

}
