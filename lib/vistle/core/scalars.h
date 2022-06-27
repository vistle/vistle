#ifndef SCALARS_H
#define SCALARS_H

#include <cstdint>
#include <vistle/util/ssize_t.h>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <array>

namespace vistle {

// list of data types usable in Vistle's shm arrays
typedef boost::mpl::vector<char, signed char, unsigned char, int32_t, uint32_t, int64_t, uint64_t, float, double>
    Scalars;
const std::array<const char *, boost::mpl::size<Scalars>::value> ScalarTypeNames = {
    {"char", "signed char", "unsigned char", "int32_t", "uint32_t", "int64_t", "uint64_t", "float", "double"}};

#define FOR_ALL_SCALARS(MACRO) \
    MACRO(char) \
    MACRO(signed char) \
    MACRO(unsigned char) \
    MACRO(int32_t) \
    MACRO(uint32_t) \
    MACRO(int64_t) \
    MACRO(uint64_t) \
    MACRO(float) \
    MACRO(double)

} // namespace vistle

#endif
