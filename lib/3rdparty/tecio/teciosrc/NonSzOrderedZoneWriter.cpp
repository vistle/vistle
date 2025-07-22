#include "NonSzOrderedZoneWriter.h"
#include "ThirdPartyHeadersBegin.h"
#include <boost/assign.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "AltTecUtil.h"
#include "checkPercentDone.h"
#include "FieldData.h"
#include "fileStuff.h"
#include "ItemSetIterator.h"
#include "writeValueArray.h"
namespace tecplot { namespace ___3931 { NonSzOrderedZoneWriter::NonSzOrderedZoneWriter( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___37&                   ___36) : NonSzZoneWriterAbstract(fileVersion, varIter, zone, ___341, ___4561, ___4496, ___36) , m_faceNeighborGenerator(___36) , m_faceNeighborWriter(m_faceNeighborGenerator, zone, ___341) {} NonSzOrderedZoneWriter::~NonSzOrderedZoneWriter() {} ___372 NonSzOrderedZoneWriter::writeZoneConnectivity(FileWriterInterface& szpltFile) { ___372 ___2037 = ___4224; m_zoneFileLocations.___2496 = ___330; if (m_writeConnectivity) { m_zoneFileLocations.___2661 = szpltFile.fileLoc(); ___2037 = m_faceNeighborWriter.write(szpltFile); } else { m_zoneFileLocations.___2661 = ___330; } return ___2037; } uint64_t NonSzOrderedZoneWriter::zoneConnectivityFileSize(bool ___2000) { if (m_writeConnectivity) return m_faceNeighborWriter.sizeInFile(___2000); else return 0; } }}
