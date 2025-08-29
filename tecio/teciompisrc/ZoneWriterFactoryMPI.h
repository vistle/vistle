 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <mpi.h>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
#include "ZoneWriterFactory.h"
namespace tecplot { namespace ___3931 { class ___37; class ZoneInfoCache; class ___4706; class ItemSetIterator; }} namespace tecplot { namespace teciompi { class ZoneWriterFactoryMPI : public ___3931::___4708 { public: ZoneWriterFactoryMPI( ___3931::ZoneInfoCache& zoneInfoCache, ___3931::___37&    ___36, MPI_Comm              communicator, int                   mainProcess); boost::shared_ptr<___3931::___4706> ___4705( uint32_t                      fileVersion, ___3931::ItemSetIterator&       varIter, ___3931::___4633            zone, ___3931::___4633            ___341, std::vector<___372> const& ___4561, ___372                     ___4496); private: MPI_Comm m_communicator; int m_mainProcess; }; }}
