#include "ZoneWriterFactoryMPI.h"
#include "ThirdPartyHeadersBegin.h"
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "AltTecUtil.h"
#include "ItemSetIterator.h"
#include "SZLFEPartitionedZoneWriterMPI.h"
#include "SZLOrderedPartitionedZoneWriterMPI.h"
#include "zoneUtil.h"
using namespace tecplot::___3931; namespace tecplot { namespace teciompi { ZoneWriterFactoryMPI::ZoneWriterFactoryMPI( ZoneInfoCache& zoneInfoCache, ___37&    ___36, MPI_Comm       communicator, int            mainProcess) : ___4708(zoneInfoCache, ___36) , m_communicator(communicator) , m_mainProcess(mainProcess) {} boost::shared_ptr<___4706> ZoneWriterFactoryMPI::___4705( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496) { REQUIRE(0 <= zone && ___2335.___4635(zone + 1)); REQUIRE(0 <= ___341 && ___341 <= zone); REQUIRE(varIter.___2810() == static_cast<___4349>(___4561.size())); REQUIRE(VALID_BOOLEAN(___4496)); ZoneType_e ___4689 = ___2335.___4617(zone + 1); if (zoneIsPartitioned(___2335, zone)) { if (___4689 == ___4701) { return boost::make_shared<SZLOrderedPartitionedZoneWriterMPI>( fileVersion, boost::ref(varIter), zone, ___341, boost::ref(___4561), ___4496, boost::ref(___2335), boost::ref(___2678), m_communicator, m_mainProcess); } else { ___476(___2335.zoneGetDimension(___2335.datasetGetUniqueID(), zone + 1) == 3); return boost::make_shared<SZLFEPartitionedZoneWriterMPI>( fileVersion, boost::ref(varIter), zone, ___341, boost::ref(___4561), ___4496, boost::ref(___2335), boost::ref(___2678), m_communicator, m_mainProcess); } } else { return ___4708::___4705(fileVersion, varIter, zone, ___341, ___4561, ___4496); } } }}
