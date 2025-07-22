 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_ptr.hpp>
#include <mpi.h>
#include <map>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "SZLOrderedPartitionedZoneWriter.h"
namespace tecplot { namespace ___3931 { class ___1879; } } namespace tecplot { namespace teciompi { class SZLOrderedPartitionedZoneWriterMPI : public ___3931::SZLOrderedPartitionedZoneWriter { public: SZLOrderedPartitionedZoneWriterMPI( uint32_t                      fileVersion, ___3931::ItemSetIterator&       varIter, ___3931::___4633            zone, ___3931::___4633            ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___3931::___37&            ___36, ___3931::ZoneInfoCache&         zoneInfoCache, MPI_Comm                      communicator, int                           mainProcess); virtual ~SZLOrderedPartitionedZoneWriterMPI(); virtual ___2477 varMinMax(___3931::___4349 datasetVar); private: virtual uint64_t zoneDataFileSize(bool ___2000); virtual uint64_t zoneHeaderFileSize(bool ___2000); virtual ___372 writeZoneData(___3931::FileWriterInterface& szpltFile); virtual ___372 writeZoneHeader(___3931::FileWriterInterface& szpltFile); struct Impl; boost::scoped_ptr<Impl> m_impl; }; }}
