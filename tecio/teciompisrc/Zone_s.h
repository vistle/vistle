 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "AuxData_s.h"
#include "basicTypes.h"
#include "CodeContract.h"
#include "FieldData_s.h"
#include "fileio.h"
#include "GhostInfo_s.h"
#include "IJK.h"
#include "NodeMap_s.h"
#include "NodeToElemMap_s.h"
namespace tecplot { namespace tecioszl { struct Zone_s { typedef std::map<___3931::___4633, boost::shared_ptr<Zone_s> > ZoneMap; typedef std::pair<___1170, ___2225> ___4604; struct ___1273 { ___372 ___2486; std::vector<___4604> ___2676; ___1273() : ___2486(___1303) {} void writeToFile(tecplot::___3931::FileWriterInterface& outputFile, bool ___4477) const { writeScalar(outputFile, ___2486, ___4477); writeScalar(outputFile, (uint64_t)___2676.size(), ___4477); for (std::vector<___4604>::const_iterator zoneCell = ___2676.begin(); zoneCell != ___2676.end(); ++zoneCell) { writeScalar(outputFile, zoneCell->first, ___4477); writeScalar(outputFile, zoneCell->second, ___4477); } } uint64_t sizeInFile(bool ___4477) const { uint64_t sizeInFile = 0; sizeInFile += scalarSizeInFile(___2486, ___4477); sizeInFile += scalarSizeInFile((uint64_t)___2676.size(), ___4477); for (std::vector<___4604>::const_iterator zoneCell = ___2676.begin(); zoneCell != ___2676.end(); ++zoneCell) { sizeInFile += scalarSizeInFile(zoneCell->first, ___4477); sizeInFile += scalarSizeInFile(zoneCell->second, ___4477); } return sizeInFile; } ___1273(tecplot::___3931::___1397& inputFile, bool readASCII) { readScalar(inputFile, ___2486, readASCII); uint64_t length; readScalar(inputFile, length, readASCII); ___2676.resize((size_t)length); for(uint64_t i = 0; i < length; ++i) { readScalar(inputFile, ___2676[i].first, readASCII); readScalar(inputFile, ___2676[i].second, readASCII); } } }; typedef std::pair<int32_t, int64_t> ___456; typedef std::map<___456, ___1273> ___1274; std::string ___2681; ZoneType_e ___2682; int32_t m_zoneDimension; tecplot::___3931::___1842 m_partitionOffset; tecplot::___3931::___1842 ___2680; std::vector<FECellShape_e> m_cellShapes; std::vector<int32_t> m_gridOrders; std::vector<FECellBasisFunction_e> m_basisFns; std::vector<int64_t> m_numElementsPerSection; double ___2619; int32_t ___2620; ___1170 ___2612; int64_t ___2501; FaceNeighborMode_e ___2456; int64_t ___2649; int64_t ___2499; int64_t ___2648; std::vector<FieldDataType_e> ___2458; std::vector<int> m_passiveVars; std::vector<ValueLocation_e> ___2668; std::vector<___1170> m_shareVarFromZone; bool m_allVarsAreShared; ___1170 m_shareConnectivityFromZone; GhostInfo_s m_ghostNodeInfo; GhostInfo_s m_ghostCellInfo; std::vector<boost::shared_ptr<___1360> > ___2494; std::vector<boost::shared_ptr<___1360> > ___2398; size_t ___2395; boost::shared_ptr<___2728> ___2495; boost::shared_ptr<___2741> m_nodeToElemMap; ___1274 ___2455; boost::shared_ptr<AuxData_s> ___2343; ZoneMap m_partitionMap; std::vector<int> m_partitionOwners; Zone_s( std::string const& ___4687, ZoneType_e ___4689, int64_t iMin,
int64_t jMin, int64_t kMin, int64_t ___1907, int64_t ___2114, int64_t ___2159, std::vector<FECellShape_e> const& cellShapes, std::vector<int32_t> const& gridOrders, std::vector<FECellBasisFunction_e> const& basisFns, std::vector<int64_t> const& numElementsPerSection, double ___3638, int32_t ___3783, ___1170 ___2972, int64_t ___2800, FaceNeighborMode_e ___1282, int64_t ___4190, int64_t ___2784, int64_t ___4186, std::vector<FieldDataType_e> const& ___1370, std::vector<int> const& passiveVarVector, std::vector<ValueLocation_e> const& ___4323, std::vector<___1170> const& shareVarFromZoneVector, ___1170 ___3547); Zone_s( Zone_s const* partitionParent, int64_t iMin, int64_t jMin, int64_t kMin, int64_t ___1907, int64_t ___2114, int64_t ___2159); void ___963(tecplot::___3931::___4349 ___4365, ___2728* ___2721); void deriveCCValues(tecplot::___3931::___4349 ___4365, ___2728* ___2721); void writeToFile(tecplot::___3931::FileWriterInterface& outputFile, bool ___4477) const; uint64_t sizeInFile(bool ___4477) const; static boost::shared_ptr<Zone_s> makePtr(tecplot::___3931::___1397& inputFile, bool readASCII); private: Zone_s(); }; }}
