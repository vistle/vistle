 #pragma once
#include "NonSzFEZoneConnectivityWriter.h"
#include "NonSzFEZoneFaceNeighborGenerator.h"
#include "NonSzZoneFaceNeighborWriter.h"
#include "NonSzZoneWriterAbstract.h"
namespace tecplot { namespace ___3931 { class NonSzFEZoneWriter : public NonSzZoneWriterAbstract { UNCOPYABLE_CLASS(NonSzFEZoneWriter); public: NonSzFEZoneWriter( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___37&                   ___36); virtual ~NonSzFEZoneWriter(); private: virtual uint64_t zoneConnectivityFileSize(bool ___2000); virtual ___372 writeZoneConnectivity(FileWriterInterface& szpltFile); NonSzFEZoneConnectivityWriter    m_connectivityWriter; ___4262 const                 m_datasetID; NonSzFEZoneFaceNeighborGenerator m_faceNeighborGenerator; NonSzZoneFaceNeighborWriter      m_faceNeighborWriter; std::string                      m_zoneNumberLabel; }; }}
