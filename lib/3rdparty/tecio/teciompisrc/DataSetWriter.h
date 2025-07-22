 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_ptr.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
#include "ZoneInfoCache.h"
#include "ZoneVarMetadata.h"
namespace tecplot { namespace ___3931 { class ___37; class FileWriterInterface; class ItemSetIterator; class DataSetWriter { public: DataSetWriter( ___37*                 ___36, ___3499                      varsToWrite, ___3499                      zonesToWrite, ___1842 const&                  maxIJKSubzoneSize, ItemAddress64::ItemOffset_t maxFESubzoneSize, bool                        flushToDisk = false); virtual ~DataSetWriter(); virtual ___372 writeDataSet( FileWriterInterface& szpltFile, uint32_t             fileVersion, ___1390&        szpltZoneHeaderFileLocs); void replaceDataSource( ___37* ___36, ___3499      varsToWrite, ___3499      zonesToWrite); void clear( int32_t        numZonesToRetain, int32_t const* zonesToRetain); ___4704 const& ___4703() { return *m_zoneVarMetadata; } protected: void getZoneSharing( std::vector<___372>& ___4561, ___372&              ___4496, ___4633             zone, ___4633             ___341, DataFileType_e          ___842) const; ___37*                        ___2335; boost::scoped_ptr<ItemSetIterator> m_varIter; boost::scoped_ptr<ItemSetIterator> m_zoneIter; ZoneInfoCache                      ___2678; boost::scoped_ptr<___4704> m_zoneVarMetadata; bool const                         m_flushingToDisk; }; }}
