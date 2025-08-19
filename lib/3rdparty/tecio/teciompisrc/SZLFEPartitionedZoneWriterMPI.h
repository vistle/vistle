 #pragma once
#include "SZLFEPartitionedZoneWriter.h"
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_ptr.hpp>
#include <mpi.h>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
namespace tecplot { namespace ___3931 { class ___37; class ZoneInfoCache; class ItemSetIterator; }} namespace tecplot { namespace teciompi { class SZLFEPartitionedZoneWriterMPI: public ___3931::SZLFEPartitionedZoneWriter { public: SZLFEPartitionedZoneWriterMPI( uint32_t                      fileVersion, ___3931::ItemSetIterator&       varIter, ___3931::___4633            zone, ___3931::___4633            ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___3931::___37&            ___36, ___3931::ZoneInfoCache&         zoneInfoCache, MPI_Comm                      communicator, int                           mainProcess); virtual ~SZLFEPartitionedZoneWriterMPI(); virtual ___2477 varMinMax(___3931::___4349 datasetVar); private: virtual uint64_t zoneDataFileSize(bool ___2000); virtual uint64_t zoneHeaderFileSize(bool ___2000); virtual ___372 writeZoneData(___3931::FileWriterInterface& szpltFile); virtual ___372 writeZoneHeader(___3931::FileWriterInterface& szpltFile); void createPartitionWriters(); void collateAndReturnResponses( class MPINonBlockingCommunicationCollection& communicationCollection); void exchangeGhostInfo(std::map<int, boost::shared_ptr<___3931::___1348> >& partitionInfoMap); struct Impl; boost::scoped_ptr<Impl> const m_impl; }; }}
