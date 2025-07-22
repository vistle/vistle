 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
namespace tecplot { namespace ___3931 { class FaceNeighborGeneratorAbstract { public: explicit FaceNeighborGeneratorAbstract(class ___37& ___36); virtual ~FaceNeighborGeneratorAbstract() {} virtual ___372 gatherUserFaceNeighbors(std::vector<int32_t>& userFaceNeighbors, ___4633 zone) const = 0; protected: void appendUserFaceNeighborsForCellFace( std::vector<int32_t>& userFaceNeighbors, ___1290 ___1272, ___3499 ___1151, ___4633 zone, int32_t ___447, int32_t face) const; class ___37& ___2335; }; }}
