 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <string>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
#include "MinMax.h"
namespace tecplot { namespace ___3931 { class ___37; class FileWriterInterface; class ItemSetIterator; class ___4706 { public: ___4706( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___37&                   ___36); virtual ~___4706(); uint64_t zoneFileSize(bool ___2000); ___372 writeZone( FileWriterInterface& szpltFile, ___1391            fileLoc); virtual uint64_t getZoneHeaderFilePosition() const; virtual ___2477 varMinMax(___4349 ___4333);
 #if defined OUTPUT_TIMES
static uint64_t ___715();
 #endif
protected: uint32_t               m_fileVersion; ItemSetIterator&       m_varIter; ___4633 const      ___2675; ___4633 const      m_baseZone; std::vector<___372> m_writeVariables; ___372 const        m_writeConnectivity; ___37&            ___2335; uint64_t               m_zoneHeaderFilePosition; uint64_t               m_zoneFileSize; private: virtual uint64_t zoneConnectivityFileSize(bool ___2000) = 0; virtual uint64_t zoneDataFileSize(bool ___2000) = 0; virtual uint64_t zoneHeaderFileSize(bool ___2000) = 0; virtual ___372 writeZoneConnectivity(FileWriterInterface& szpltFile) = 0; virtual ___372 writeZoneData(FileWriterInterface& szpltFile) = 0; virtual ___372 writeZoneHeader(FileWriterInterface& szpltFile) = 0; }; }}
