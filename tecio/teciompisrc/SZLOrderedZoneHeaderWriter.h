 #pragma once
#include "SzlFileLoader.h"
#include "ZoneHeaderWriterAbstract.h"
namespace tecplot { namespace ___3931 { class ___37; class ___1879; class ItemSetIterator; class SZLOrderedZoneHeaderWriter : public ZoneHeaderWriterAbstract { public: SZLOrderedZoneHeaderWriter( uint32_t            fileVersion, ItemSetIterator&    varIter, ___4633         zone, ___4633         ___341, ___37&         ___36, ___1879 const&  ijkZoneInfo, ___1390 const& varFileLocs); virtual ~SZLOrderedZoneHeaderWriter(); virtual uint64_t sizeInFile(bool ___2000) const; virtual ___372 write(FileWriterInterface& fileWriter) const; private: ___1879 const& m_ijkZoneInfo; ___1390 const& ___2671; }; }}
