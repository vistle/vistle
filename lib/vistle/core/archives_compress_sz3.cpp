/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_oarchive.cpp:
// binary_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include "archives_compress_sz3.h"

#ifdef HAVE_SZ3
#include <SZ3/api/sz.hpp>
#endif


//#define COMP_DEBUG

namespace vistle {

#ifdef HAVE_SZ3
namespace detail {

template<>
char *compressSz3<void>(size_t &compressedSize, const void *src, const SZ::Config &conf)
{
    compressedSize = 0;
    return nullptr;
}

template<>
char *compressSz3<float>(size_t &compressedSize, const float *src, const SZ::Config &conf)
{
    compressedSize = 0;
    char *buf = SZ_compress(conf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(float) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<double>(size_t &compressedSize, const double *src, const SZ::Config &conf)
{
    compressedSize = 0;
    char *buf = SZ_compress(conf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(double) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const SZ::Config &conf)
{
    compressedSize = 0;
    char *buf = SZ_compress(conf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(int32_t) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const SZ::Config &conf)
{
    compressedSize = 0;
    char *buf = SZ_compress(conf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(int64_t) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
bool decompressSz3<void>(void *dest, const buffer &compressed, const Index dim[3])
{
    return false;
}

template<>
bool decompressSz3<float>(float *dest, const buffer &compressed, const Index dim[3])
{
    SZ::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3])
{
    SZ::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3])
{
    SZ::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3])
{
    SZ::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template char *compressSz3<void>(size_t &compressedSize, const void *src, const SZ::Config &conf);
template char *compressSz3<float>(size_t &compressedSize, const float *src, const SZ::Config &conf);
template char *compressSz3<double>(size_t &compressedSize, const double *src, const SZ::Config &conf);
template char *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const SZ::Config &conf);
template char *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const SZ::Config &conf);

} // namespace detail
#endif // HAVE_SZ3

} // namespace vistle
