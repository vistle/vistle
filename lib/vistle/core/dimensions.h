#ifndef VISTLE_CORE_DIMENSIONS_H
#define VISTLE_CORE_DIMENSIONS_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/find.hpp>

namespace vistle {

typedef boost::mpl::vector_c<int, 1, 2, 3, 4> Dimensions;
const int MaxDimension = 4;

} // namespace vistle

#endif
