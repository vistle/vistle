/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_oarchive.cpp:
// binary_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#ifdef HAVE_SZ3
#include <SZ3/utils/Config.hpp>
#include <SZ3/api/sz.hpp>
#endif

#include "archives_compress_sz3.h"
#include <cassert>


//#define COMP_DEBUG

namespace vistle {

#ifdef HAVE_SZ3
namespace detail {

SZ3::Config getConfig(const CompressionSettings &cs, const Index dim[3])
{
    assert(cs.mode == SZ);

    std::vector<size_t> dims;
    for (int c = 0; c < 3; ++c)
        dims.push_back(dim[c]);
    while (dims.back() == 1 && dims.size() > 1)
        dims.pop_back();
    SZ3::Config conf;
    if (dims.size() == 1)
        conf = SZ3::Config(dims[0]);
    else if (dims.size() == 2)
        conf = SZ3::Config(dims[0], dims[1]);
    else if (dims.size() == 3)
        conf = SZ3::Config(dims[0], dims[1], dims[2]);
    switch (cs.szAlgo) {
    case SzInterp:
        conf.cmprAlgo = SZ3::ALGO_INTERP;
        break;
    case SzInterpLorenzo:
        conf.cmprAlgo = SZ3::ALGO_INTERP_LORENZO;
        break;
    case SzLorenzoReg:
        conf.cmprAlgo = SZ3::ALGO_LORENZO_REG;
        break;
    }
    switch (cs.szError) {
    case SzAbs:
        conf.errorBoundMode = SZ3::EB_ABS;
        break;
    case SzRel:
        conf.errorBoundMode = SZ3::EB_REL;
        break;
    case SzPsnr:
        conf.errorBoundMode = SZ3::EB_PSNR;
        break;
    case SzL2:
        conf.errorBoundMode = SZ3::EB_L2NORM;
        break;
    case SzAbsAndRel:
        conf.errorBoundMode = SZ3::EB_ABS_AND_REL;
        break;
    case SzAbsOrRel:
        conf.errorBoundMode = SZ3::EB_ABS_OR_REL;
        break;
    }
    conf.absErrorBound = cs.szAbsError;
    conf.relErrorBound = cs.szRelError;
    conf.psnrErrorBound = cs.szPsnrError;
    conf.l2normErrorBound = cs.szL2Error;
    conf.encoder = 0;
    conf.lossless = 0;

    return conf;
}

template<>
char *compressSz3<void>(size_t &compressedSize, const void *src, const Index dim[3], const CompressionSettings &conf)
{
    compressedSize = 0;
    return nullptr;
}

template<>
char *compressSz3<float>(size_t &compressedSize, const float *src, const Index dim[3], const CompressionSettings &conf)
{
    SZ3::Config szconf = getConfig(conf, dim);
    compressedSize = 0;
    char *buf = SZ_compress(szconf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(float) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<double>(size_t &compressedSize, const double *src, const Index dim[3],
                          const CompressionSettings &conf)
{
    SZ3::Config szconf = getConfig(conf, dim);
    compressedSize = 0;
    char *buf = SZ_compress(szconf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(double) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const Index dim[3],
                           const CompressionSettings &conf)
{
    SZ3::Config szconf = getConfig(conf, dim);
    compressedSize = 0;
    char *buf = SZ_compress(szconf, src, compressedSize);
#ifdef COMP_DEBUG
    std::cerr << "compressSz3: compressed " << conf.num << " elements (dim " << int(conf.N) << "), size "
              << conf.num * sizeof(int32_t) << " to " << compressedSize << " bytes" << std::endl;
#endif
    return buf;
}

template<>
char *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const Index dim[3],
                           const CompressionSettings &conf)
{
    SZ3::Config szconf = getConfig(conf, dim);
    compressedSize = 0;
    char *buf = SZ_compress(szconf, src, compressedSize);
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
    SZ3::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3])
{
    SZ3::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3])
{
    SZ3::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

template<>
bool decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3])
{
    SZ3::Config conf;
    SZ_decompress(conf, const_cast<char *>(compressed.data()), compressed.size(), dest);
    return true;
}

} // namespace detail
#endif // HAVE_SZ3

} // namespace vistle
