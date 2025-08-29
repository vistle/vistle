 #pragma once
#include "ClassMacros.h"
#include "NonSzOrderedZoneFaceNeighborGenerator.h"
#include "NonSzZoneFaceNeighborWriter.h"
#include "NonSzZoneWriterAbstract.h"
namespace tecplot { namespace ___3931 { class ItemSetIterator; class NonSzOrderedZoneWriter : public NonSzZoneWriterAbstract { UNCOPYABLE_CLASS(NonSzOrderedZoneWriter); public: NonSzOrderedZoneWriter( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496, ___37&                   ___36); virtual ~NonSzOrderedZoneWriter(); private: virtual uint64_t zoneConnectivityFileSize(bool ___2000); virtual ___372 writeZoneConnectivity(FileWriterInterface& szpltFile); NonSzOrderedZoneFaceNeighborGenerator m_faceNeighborGenerator; NonSzZoneFaceNeighborWriter m_faceNeighborWriter; ___372 ___4511( FileWriterInterface& file, ValueLocation_e      ___4323, ___4349           ___4333); }; }}
