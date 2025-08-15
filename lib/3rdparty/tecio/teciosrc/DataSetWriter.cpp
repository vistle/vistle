#include "DataSetWriter.h"
#include "ThirdPartyHeadersBegin.h"
#include <boost/unordered_set.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "AltTecUtil.h"
#include "CodeContract.h"
#include "FileWriterInterface.h"
#include "ItemSetIterator.h"
#include "SzlFileLoader.h"
#include "ZoneWriterAbstract.h"
#include "ZoneWriterFactory.h"
#include "zoneUtil.h"
#include "ZoneVarMetadata.h"
namespace tecplot { namespace ___3931 { DataSetWriter::DataSetWriter( ___37*                 ___36, ___3499                      varsToWrite, ___3499                      zonesToWrite, ___1842 const&                  maxIJKSubzoneSize, ItemAddress64::ItemOffset_t maxFESubzoneSize, bool                        flushingToDisk  ) : ___2335(___36) , m_varIter(new ItemSetIterator(*___36, ___36->___894() ? ___36->___888() : 0, varsToWrite)) , m_zoneIter(new ItemSetIterator(*___36, ___36->___894() ? ___36->___889() : 0, zonesToWrite)) , ___2678(___36, maxIJKSubzoneSize, maxFESubzoneSize) , m_zoneVarMetadata(new ___4704(*___36, *m_varIter, *m_zoneIter, flushingToDisk  )) , m_flushingToDisk(flushingToDisk) { REQUIRE(VALID_REF(___36)); } DataSetWriter::~DataSetWriter() {} ___372 DataSetWriter::writeDataSet( FileWriterInterface& szpltFile, uint32_t             fileVersion, ___1390&        szpltZoneHeaderFileLocs) { REQUIRE(szpltFile.___2039()); REQUIRE(fileVersion == 105 || fileVersion == 231 || fileVersion == 232); if (!___2335->___894()) return ___4224; ___372 ___2037 = ___4224; m_zoneIter->reset(); ___4633 const ___341 = m_zoneIter->baseItem(); while (___2037 && m_zoneIter->hasNext()) { ___4633 const ___902 = m_zoneIter->next(); if (!___2335->___4635(___902+1)) continue; try { std::vector<___372> ___4561; ___372 ___4496; getZoneSharing(___4561, ___4496, ___902, ___341, szpltFile.___842());
 #if defined OUTPUT_TIMES
uint64_t ___3685 = ___4706::___715();
 #endif
___4708 ___4707(___2678, *___2335); boost::shared_ptr<___4706> ___4705 = ___4707.___4705(fileVersion, *m_varIter, ___902, m_flushingToDisk ? 0 : ___341, ___4561, ___4496);
 #if defined OUTPUT_TIMES
uint64_t ___1165 = ___4706::___715(); ___1929(NULL, "%g seconds partitioning zone.", (double)(___1165 - ___3685) / 1000.0);
 #endif
___2037 = ___4705->writeZone(szpltFile, szpltFile.fileLoc()); ___4633 const fileZone = ___902 - ___341; if (___2037) { szpltZoneHeaderFileLocs[fileZone] = ___4705->getZoneHeaderFilePosition();
 #if defined DO_SUBZONE_HISTOGRAM || defined DO_ITEMANDSUBZONE_HISTOGRAM
{ if (___4641(*___2335, ___902) && !zoneIsPartitioned(*___2335, ___902)) { boost::shared_ptr<___1348> zoneInfo = ___2678.getFEZoneInfo(___902, ___341);
 #ifdef DO_SUBZONE_HISTOGRAM
extern ___372 OutputSubzoneHistograms( char const*       szpltFileName, ___37&       ___36, ___4633       zone, boost::shared_ptr<___1348 const> ___1347); OutputSubzoneHistograms(szpltFile.___1392().c_str(), *___2335, ___902, zoneInfo);
 #endif
 #ifdef DO_ITEMANDSUBZONE_HISTOGRAM
extern ___372 OutputItemAndSubzoneHistograms( char const*       szpltFileName, ___37&       ___36, ___4633       zone, boost::shared_ptr<___1348 const> ___1347); OutputItemAndSubzoneHistograms(szpltFile.___1392().c_str(), *___2335, ___902, zoneInfo);
 #endif
} }
 #endif
} m_varIter->reset(); ___4349 const baseVar = m_varIter->baseItem(); while (m_varIter->hasNext()) { ___4349 const datasetVar = m_varIter->next(); ___4349 const fileVar = datasetVar - baseVar; m_zoneVarMetadata->m_vzMinMaxes[fileVar][fileZone] = ___4705->varMinMax(datasetVar); } if (szpltFile.___842() == ___843 && !m_flushingToDisk) ___2678.remove(___902); } catch(std::exception const& e) { ___2037 = ___1184(e.what()); } if (!___2037) ___2678.clear(); } return ___2037; } void DataSetWriter::replaceDataSource( ___37* ___36, ___3499      varsToWrite, ___3499      zonesToWrite) { REQUIRE(VALID_REF_OR_NULL(___36)); ___2335 = ___36; ___2678.replaceDataSource(___36, zonesToWrite); if (___36 == NULL) { m_varIter.reset(); m_zoneIter.reset(); m_zoneVarMetadata.reset(); } else { m_varIter.reset(new ItemSetIterator(*___2335, ___36->___894() ? ___36->___888() : 0, varsToWrite)); m_zoneIter.reset(new ItemSetIterator(*___2335, ___36->___894() ? ___36->___889() : 0, zonesToWrite)); m_zoneVarMetadata.reset(new ___4704(*___36, *m_varIter, *m_zoneIter, m_flushingToDisk)); } } void DataSetWriter::clear( int32_t        numZonesToRetain, int32_t const* zonesToRetain) { REQUIRE(IMPLICATION(numZonesToRetain > 0, VALID_REF(zonesToRetain))); if (numZonesToRetain == 0) { ___2678.clear(); } else { boost::unordered_set<int32_t> retainZoneSet; for (int32_t i = 0; i < numZonesToRetain; ++i) retainZoneSet.insert(zonesToRetain[i]); m_zoneIter->reset(); while (m_zoneIter->hasNext()) { ___4633 const ___902 = m_zoneIter->next(); if (___2335->___4635(___902 + 1) && retainZoneSet.find(___902 + 1) == retainZoneSet.end()) ___2678.remove(___902); } } } void DataSetWriter::getZoneSharing( std::vector<___372>& ___4561, ___372&              ___4496, ___4633             ___902, ___4633             ___341, DataFileType_e          ___842) const { REQUIRE(0 <= ___902 && ___2335->___4635(___902 + 1)); REQUIRE(0 <= ___341 && ___341 <= ___902); REQUIRE(___4561.empty()); m_varIter->reset(); ___4349 const baseVar = m_varIter->baseItem(); ___4633 const fileZone = ___902 - ___341; while (m_varIter->hasNext()) { ___4349 const fileVar = m_varIter->next() - baseVar; ___4561.push_back( !m_zoneVarMetadata->m_vzIsPassive[fileVar][fileZone] && m_zoneVarMetadata->m_vzShareVarWithZone[fileVar][fileZone] == -1); } ___4496 = (___842 != ___846 && m_zoneVarMetadata->m_zoneShareConnectivityWithZone[fileZone] == -1); } }}
