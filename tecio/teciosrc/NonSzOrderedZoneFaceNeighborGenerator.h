 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
#include "FaceNeighborGeneratorAbstract.h"
namespace tecplot { namespace ___3931 { class ___37; class NonSzOrderedZoneFaceNeighborGenerator : public FaceNeighborGeneratorAbstract { public: explicit NonSzOrderedZoneFaceNeighborGenerator(___37& ___36); ___372 gatherUserFaceNeighbors(std::vector<int32_t>& userFaceNeighbors, ___4633 zone) const; }; }}
