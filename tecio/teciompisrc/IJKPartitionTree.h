 #pragma once
 #if defined (___1987)
 #undef ___1987 
 #endif
#include "ThirdPartyHeadersBegin.h"
#include <iterator>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include "ThirdPartyHeadersEnd.h"
 #if defined (___1987)
 #undef ___1987
 #endif
 #define ___1987 (-1)
#include "IJK.h"
#include "ItemAddress.h"
namespace tecplot { namespace ___3931 { int const RTREE_PAGE_SIZE = 8; typedef boost::geometry::model::point<int64_t, 3, boost::geometry::cs::cartesian> ___1851; typedef boost::geometry::model::box<___1851> ___1853; typedef std::pair<___1853, tecplot::ItemAddress64::___2978> ___1862; class ___1861 : public boost::geometry::index::rtree<___1862, boost::geometry::index::quadratic<RTREE_PAGE_SIZE> > { public: ___1861() {} explicit ___1861(std::vector<___1862> const& ___2979) : boost::geometry::index::rtree<___1862, boost::geometry::index::quadratic<RTREE_PAGE_SIZE> >(___2979) {} void ___13(ItemAddress64::___2978 ___2975, ___1842 const& ___2472, ___1842 const& ___2362) { ___1851 ___2476(___2472.i(), ___2472.___2103(), ___2472.___2132()); ___1851 ___2370(___2362.i(), ___2362.___2103(), ___2362.___2132()); insert(std::make_pair(___1853(___2476, ___2370), ___2975)); } bool find(___1842 const& ___2472, ___1842 const& ___2362, std::vector<___1862>& ___2097) { REQUIRE(___2097.empty()); ___1851 ___2476(___2472.i(), ___2472.___2103(), ___2472.___2132()); ___1851 ___2370(___2362.i(), ___2362.___2103(), ___2362.___2132()); query(boost::geometry::index::intersects(___1853(___2476, ___2370)), std::back_inserter(___2097)); return !___2097.empty(); } }; }}
