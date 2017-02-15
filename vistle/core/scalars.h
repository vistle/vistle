#ifndef SCALARS_H
#define SCALARS_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <array>

namespace vistle {

// list of data types usable in Vistle's shm arrays
typedef boost::mpl::vector<unsigned char, int, unsigned int, size_t, ssize_t, float, double> Scalars;
const std::array<const char*, boost::mpl::size<Scalars>::value> ScalarTypeNames = {{"unsigned char", "int", "unsigned int", "size_t", "ssize_t", "float", "double"}};

}

#endif
