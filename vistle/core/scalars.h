#ifndef SCALARS_H
#define SCALARS_H

#include <boost/mpl/vector.hpp>
#include <array>

namespace vistle {

typedef boost::mpl::vector<unsigned char, int, unsigned int, size_t, float, double> Scalars;
const std::array<const char*, 6> ScalarTypeNames = {{"unsigned char", "int", "unsigned int", "size_t", "float", "double"}};

}

#endif
