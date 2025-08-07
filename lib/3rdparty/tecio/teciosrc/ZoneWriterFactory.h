 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "basicTypes.h"
namespace tecplot { namespace ___3931 { class ___37; class ZoneInfoCache; class ___4706; class ItemSetIterator; class ___4708 { public: ___4708( ZoneInfoCache& zoneInfoCache, ___37& ___36); boost::shared_ptr<___4706> ___4705( uint32_t                      fileVersion, ItemSetIterator&              varIter, ___4633                   zone, ___4633                   ___341, std::vector<___372> const& ___4561, ___372                     ___4496); protected: ZoneInfoCache& ___2678; ___37& ___2335; }; }}
