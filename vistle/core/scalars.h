#ifndef SCALARS_H
#define SCALARS_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <array>
#include <util/ssize_t.h>

namespace vistle {

// list of data types usable in Vistle's shm arrays
typedef boost::mpl::vector<char, signed char, unsigned char, int32_t, uint32_t, int64_t, uint64_t, float, double> Scalars;
const std::array<const char*, boost::mpl::size<Scalars>::value> ScalarTypeNames = {{"char", "signed char", "unsigned char", "int32_t", "uint32_t", "int64_t", "uint64_t", "float", "double"}};

}

#endif
