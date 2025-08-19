#include "ZoneWriterAbstract.h"
#include "AltTecUtil.h"
#include "ThirdPartyHeadersBegin.h"
#include <stdexcept>
#include <boost/assign.hpp>
 #if defined OUTPUT_TIMES
 #if defined _WINDOWS
#include <sys\types.h>
#include <sys\timeb.h>
 #else
#include <sys/time.h>
 #endif
 #endif
#include "ThirdPartyHeadersEnd.h"
#include "ItemSetIterator.h"
#include "writeValueArray.h"
namespace tecplot { namespace ___3931 { ___4706::___4706( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___37&                   ___36) : m_fileVersion(fileVersion) , m_varIter(varIter) , ___2675(zone) , m_baseZone(___341) , m_writeVariables(___4561) , m_writeConnectivity(___4496) , ___2335(___36) , m_zoneHeaderFilePosition(___330) , m_zoneFileSize(0) { ___476(0 <= m_baseZone && m_baseZone <= ___2675); if (!___2335.___894()) throw std::logic_error("Attempted to write zone for non-existent data set."); else if (___2675 < 0 || !___2335.___4635(___2675 + 1)) throw std::logic_error("Attempted to write non-existent zone."); } ___4706::~___4706() {} uint64_t ___4706::zoneFileSize(bool ___2000) { if (!m_zoneFileSize) m_zoneFileSize = zoneConnectivityFileSize(___2000) + zoneDataFileSize(___2000) + zoneHeaderFileSize(___2000); return m_zoneFileSize; } ___372 ___4706::writeZone( FileWriterInterface& szpltFile, ___1391            fileLoc) { ASSERT_ONLY(uint64_t fileSize1 = fileLoc); ASSERT_ONLY(uint64_t predictedFileSize2 = fileSize1 + zoneConnectivityFileSize(szpltFile.___2000() == ___4224)); szpltFile.___3457(fileLoc); if (writeZoneConnectivity(szpltFile)) { ASSERT_ONLY(uint64_t fileSize2 = szpltFile.fileLoc()); ___476(fileSize2 == predictedFileSize2); ASSERT_ONLY(uint64_t predictedFileSize3 = fileSize2 + zoneDataFileSize(szpltFile.___2000() == ___4224)); if (writeZoneData(szpltFile)) { ASSERT_ONLY(uint64_t fileSize3 = szpltFile.fileLoc()); ___476(fileSize3 == predictedFileSize3); ASSERT_ONLY(uint64_t predictedFileSize4 = fileSize3 + zoneHeaderFileSize(szpltFile.___2000() == ___4224)); m_zoneHeaderFilePosition = szpltFile.fileLoc(); ___372 ___3356 = writeZoneHeader(szpltFile); ASSERT_ONLY(uint64_t fileSize4 = szpltFile.fileLoc()); ___476(fileSize4 == predictedFileSize4); return ___3356; } } return ___1303; } uint64_t ___4706::getZoneHeaderFilePosition() const { if (m_zoneHeaderFilePosition == ___330) throw std::runtime_error("Internal error: getZoneHeaderFilePosition() called before writeZone()."); return m_zoneHeaderFilePosition; } ___2477 ___4706::varMinMax(___4349 datasetVar) { REQUIRE(m_varIter.baseItem() <= datasetVar && datasetVar - m_varIter.baseItem() < m_varIter.___2810()); double minValue; double maxValue; ___2335.___911(___2675 + 1, datasetVar + 1, &minValue, &maxValue); return ___2477(minValue, maxValue); }
 #if defined OUTPUT_TIMES
uint64_t ___4706::___715(void) {
 #if defined UNIXX
 #  if defined LINUXALPHA
{ struct timeb now; ftime(&now); return now.time * 1000L + (long)now.millitm; }
 #  elif defined LINUX
{ struct timeval now; gettimeofday(&now, NULL); return now.tv_sec * 1000L + now.tv_usec / 1000L; }
 #  else
{ struct timeval now; struct timezone ___4177; gettimeofday(&now, &___4177); return now.tv_sec * 1000L + now.tv_usec / 1000L; }
 #  endif
 #endif 
 #if defined _WIN32
{ struct _timeb curTimeB; _ftime(&curTimeB); return curTimeB.time * 1000LL + curTimeB.millitm; }
 #endif 
}
 #endif
}}
