 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <mpi.h>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "AltTecUtil.h"
#include "basicTypes.h"
#include "StandardIntegralTypes.h"
namespace tecplot { namespace teciompi { bool everyRankAppearsOnce(MPI_Comm comm, std::vector<int32_t> const& ranks); bool everyRankOwnsOnePartition(MPI_Comm comm, ___3931::___37& ___36, ___3931::___4633 ___4655); void gatherScatterPartitionFileLocs(___3931::___1391& finalFileLoc, ___3931::___1391& localPartitionFileLoc, ___3931::___37 &___36, ___3931::___4633 const zone, int32_t localProcess, uint64_t localPartitionFileSize, int32_t mainProcess, MPI_Comm comm); }}
