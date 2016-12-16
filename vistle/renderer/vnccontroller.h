#ifndef VISTLE_VNCCONTROLLER_H
#define VISTLE_VNCCONTROLLER_H

#include "renderer.h"
#include <rhr/vncserver.h>

namespace vistle {

class V_RENDEREREXPORT VncController {

 public:
   VncController(vistle::Module *module, int displayRank);
   bool handleParam(const vistle::Parameter *p);
   std::shared_ptr<VncServer> server() const;
   int rootRank() const;

 private:
   vistle::Module *m_module;
   int m_displayRank;

   IntParameter *m_vncOnSlaves;

   IntParameter *m_vncBasePort;
   IntParameter *m_vncForward;
   unsigned short m_forwardPort; //< current port mapping

   IntParameter *m_vncReverse;
   StringParameter *m_vncRevHost;
   IntParameter *m_vncRevPort;

   IntParameter *m_rgbaEncoding;
   VncServer::ColorCodec m_rgbaCodec;
   IntParameter *m_depthPrec;
   Integer m_prec;
   IntParameter *m_depthQuant;
   bool m_quant;
   IntParameter *m_depthSnappy;
   bool m_snappy;

   IntVectorParameter *m_sendTileSizeParam;
   IntParamVector m_sendTileSize;

   std::shared_ptr<VncServer> m_vnc;
};

}
#endif

