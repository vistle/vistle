#ifndef VISTLE_RHRCONTROLLER_H
#define VISTLE_RHRCONTROLLER_H

#include "renderer.h"
#include <rhr/rhrserver.h>

namespace vistle {

class V_RENDEREREXPORT RhrController {

 public:
   RhrController(vistle::Module *module, int displayRank);
   bool handleParam(const vistle::Parameter *p);
   std::shared_ptr<RhrServer> server() const;
   int rootRank() const;

 private:
   vistle::Module *m_module;
   int m_displayRank;

   IntParameter *m_rhrBasePort;
   IntParameter *m_rhrForward;
   unsigned short m_forwardPort; //< current port mapping

   IntParameter *m_rgbaEncoding;
   RhrServer::ColorCodec m_rgbaCodec;
   IntParameter *m_depthPrec;
   Integer m_prec;
   IntParameter *m_depthQuant;
   bool m_quant;
   IntParameter *m_depthSnappy;
   bool m_snappy;

   IntVectorParameter *m_sendTileSizeParam;
   IntParamVector m_sendTileSize;

   std::shared_ptr<RhrServer> m_rhr;
};

}
#endif

