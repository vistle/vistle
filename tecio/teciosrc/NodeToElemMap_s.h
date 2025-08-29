 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <boost/scoped_array.hpp>
#include "ThirdPartyHeadersEnd.h"
#include "MASTER.h"
#include "GLOBAL.h"
#include "CodeContract.h"
#include "NodeMap_s.h"
namespace tecplot { namespace tecioszl { struct ___2741 { tecplot::___3931::___2716 m_nodeCount; boost::scoped_array<tecplot::___3931::___463> m_elemIndex; boost::scoped_array<tecplot::___3931::___463> m_elem; ___2741(___2728 const& ___2721, tecplot::___3931::___2716 nodeCount); tecplot::___3931::___463 cellCountForNode(tecplot::___3931::___2716 ___2707); }; }}
