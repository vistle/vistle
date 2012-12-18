#ifndef SCALARS_H
#define SCALARS_H

#include <boost/mpl/vector.hpp>

namespace vistle {

typedef boost::mpl::vector<unsigned char, int, size_t, float, double> Scalars;

}

#endif
