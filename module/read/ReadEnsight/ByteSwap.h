#ifndef READENSIGHT_BYTESWAP_H
#define READENSIGHT_BYTESWAP_H

#include <vistle/util/byteswap.h>

template<typename T>
void byteSwap(T &t)
{
    t = vistle::byte_swap<vistle::little_endian, vistle::big_endian, T>(t);
}

template<typename T>
void byteSwap(T *t, size_t n)
{
    for (size_t i = 0; i < n; ++n) {
        *t = vistle::byte_swap<vistle::little_endian, vistle::big_endian, T>(*t);
        ++t;
    }
}

#endif
