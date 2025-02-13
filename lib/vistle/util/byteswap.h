#ifndef VISTLE_UTIL_BYTESWAP_H
#define VISTLE_UTIL_BYTESWAP_H

#define _USE_BUILTIN_BSWAPS

// from Virvo
// from http://www.cplusplus.com/forum/general/27544/


#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/version.hpp>
#include <boost/predef/other/endian.h>
#include <stdexcept>

//#define UNDEFINED_BEHAVIOR // might be faster

namespace vistle {

// Little-endian operating systems:
//---------------------------------
// Linux on x86, x64, Alpha and Itanium
// Mac OS on x86, x64
// Solaris on x86, x64, PowerPC
// Tru64 on Alpha
// Windows on x86, x64 and Itanium

// Big-endian operating systems:
//------------------------------
// AIX on POWER
// AmigaOS on PowerPC and 680x0
// HP-UX on Itanium and PA-RISC
// Linux on MIPS, SPARC, PA-RISC, POWER, PowerPC, and 680x0
// Mac OS on PowerPC and 680x0
// Solaris on SPARC

enum endianness {
    little_endian,
    big_endian,
    network_endian = big_endian,

#if BOOST_ENDIAN_LITTLE_BYTE
    host_endian = little_endian
#elif BOOST_ENDIAN_BIG_BYTE
    host_endian = big_endian
#else
#error "unable to determine system endianness"
#endif
};

namespace detail {

template<typename T, size_t sz>
struct swap_bytes {
    inline T operator()(T val)
    {
        (void)val;
        throw std::out_of_range("data size");
    }
};

template<typename T>
struct swap_bytes<T, 1> {
    inline T operator()(T val) { return val; }
};

template<typename T>
struct swap_bytes<T, 2> // for 16 bit
{
    inline T operator()(T val) { return ((((val) >> 8) & 0xff) | (((val)&0xff) << 8)); }
};

template<typename T>
struct swap_bytes<T, 4> // for 32 bit
{
    inline T operator()(T val)
    {
#if defined(_USE_BUILTIN_BSWAPS) && defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __GNUC__ > 4)
        return __builtin_bswap32(val);
#else
        return ((((val)&0xff000000) >> 24) | (((val)&0x00ff0000) >> 8) | (((val)&0x0000ff00) << 8) |
                (((val)&0x000000ff) << 24));
#endif
    }
};

template<>
struct swap_bytes<float, 4> {
    inline float operator()(float val)
    {
#ifdef UNDEFINED_BEHAVIOR
        uint32_t mem = swap_bytes<uint32_t, sizeof(uint32_t)>()(*(uint32_t *)&val);
        return *(float *)&mem;
#else
        char *in = reinterpret_cast<char *>(&val);
        char *out = in + 3;
        while (in < out) {
            std::swap(*in, *out);
            ++in;
            --out;
        }
        return val;
#endif
    }
};

template<typename T>
struct swap_bytes<T, 8> // for 64 bit
{
    inline T operator()(T val)
    {
#if defined(_USE_BUILTIN_BSWAPS) && defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __GNUC__ > 4)
        return __builtin_bswap64(val);
#else
        return ((((val)&0xff00000000000000ull) >> 56) | (((val)&0x00ff000000000000ull) >> 40) |
                (((val)&0x0000ff0000000000ull) >> 24) | (((val)&0x000000ff00000000ull) >> 8) |
                (((val)&0x00000000ff000000ull) << 8) | (((val)&0x0000000000ff0000ull) << 24) |
                (((val)&0x000000000000ff00ull) << 40) | (((val)&0x00000000000000ffull) << 56));
#endif
    }
};

template<>
struct swap_bytes<double, 8> {
    inline double operator()(double val)
    {
#ifdef UNDEFINED_BEHAVIOR
        uint64_t mem = swap_bytes<uint64_t, sizeof(uint64_t)>()(*(uint64_t *)&val);
        return *(double *)&mem;
#else
        char *in = reinterpret_cast<char *>(&val);
        char *out = in + 7;
        while (in < out) {
            std::swap(*in, *out);
            ++in;
            --out;
        }
        return val;
#endif
    }
};

template<endianness from, endianness to, class T>
struct do_byte_swap {
    inline T operator()(T value) { return swap_bytes<T, sizeof(T)>()(value); }
};
// specialisations when attempting to swap to the same endianness
template<class T>
struct do_byte_swap<little_endian, little_endian, T> {
    inline T operator()(T value) { return value; }
};
template<class T>
struct do_byte_swap<big_endian, big_endian, T> {
    inline T operator()(T value) { return value; }
};

} // namespace detail

template<endianness from, endianness to, class T>
inline T byte_swap(T value)
#ifdef __GNUC__
    __attribute__((warn_unused_result))
#endif
    ;

template<endianness from, endianness to, class T>
inline T byte_swap(T value)
{
    // ensure the data is only 1, 2, 4 or 8 bytes
    BOOST_STATIC_ASSERT(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
    // ensure we're only swapping arithmetic types
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);

    return detail::do_byte_swap<from, to, T>()(value);
}

} // namespace vistle
#endif
