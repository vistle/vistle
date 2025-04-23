#ifndef VISTLE_CORE_SCALARS_H
#define VISTLE_CORE_SCALARS_H

#include <cstdint>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <array>

namespace vistle {

// list of data types usable in Vistle's shm arrays
typedef boost::mpl::vector<char, signed char, unsigned char, int32_t, uint32_t, int64_t, uint64_t, float, double>
    Scalars;
const std::array<const char *, boost::mpl::size<Scalars>::value> ScalarTypeNames = {
    {"char", "int8_t", "uint8_t", "int32_t", "uint32_t", "int64_t", "uint64_t", "float", "double"}};
template<typename S>
unsigned scalarTypeId();

#define FOR_ALL_SCALARS(MACRO) \
    MACRO(char) \
    MACRO(int8_t) \
    MACRO(uint8_t) \
    MACRO(int32_t) \
    MACRO(uint32_t) \
    MACRO(int64_t) \
    MACRO(uint64_t) \
    MACRO(float) \
    MACRO(double)

template<typename S>
struct can_load {
    typedef boost::mpl::vector<S> list;
};

template<>
struct can_load<int64_t> {
    typedef boost::mpl::vector<int64_t, int32_t> list;
};

template<>
struct can_load<uint64_t> {
    typedef boost::mpl::vector<uint64_t, uint32_t> list;
};

template<>
struct can_load<float> {
    typedef boost::mpl::vector<float, double> list;
};

template<>
struct can_load<double> {
    typedef boost::mpl::vector<double, float> list;
};

} // namespace vistle

#endif
