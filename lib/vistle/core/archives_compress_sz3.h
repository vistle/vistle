#ifndef VISTLE_ARCHIVES_COMPRESS_SZ3_H
#define VISTLE_ARCHIVES_COMPRESS_SZ3_H

#ifdef HAVE_SZ3
#include <SZ3/utils/Config.hpp>
#endif

#include <vistle/util/buffer.h>
#include "index.h"
#include "export.h"

namespace vistle {

namespace detail {

#ifdef HAVE_SZ3
template<typename T>
char *compressSz3(size_t &compressedSize, const T *src, const SZ3::Config &conf);

template<typename T>
bool decompressSz3(T *dest, const buffer &compressed, const Index dim[3]);

template<>
char V_COREEXPORT *compressSz3<void>(size_t &compressedSize, const void *src, const SZ3::Config &conf);
template<>
char V_COREEXPORT *compressSz3<float>(size_t &compressedSize, const float *src, const SZ3::Config &conf);
template<>
char V_COREEXPORT *compressSz3<double>(size_t &compressedSize, const double *src, const SZ3::Config &conf);
template<>
char V_COREEXPORT *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const SZ3::Config &conf);
template<>
char V_COREEXPORT *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const SZ3::Config &conf);
template<>
bool V_COREEXPORT decompressSz3<void>(void *dest, const buffer &compressed, const Index dim[3]);
template<>
bool V_COREEXPORT decompressSz3<float>(float *dest, const buffer &compressed, const Index dim[3]);
template<>
bool V_COREEXPORT decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3]);
template<>
bool V_COREEXPORT decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3]);
template<>
bool V_COREEXPORT decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3]);

#if 0
extern template char *compressSz3<void>(size_t &compressedSize, const void *src, const SZ3::Config &conf);
extern template char *compressSz3<float>(size_t &compressedSize, const float *src, const SZ3::Config &conf);
extern template char *compressSz3<double>(size_t &compressedSize, const double *src, const SZ3::Config &conf);
extern template char *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const SZ3::Config &conf);
extern template char *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const SZ3::Config &conf);

extern template bool decompressSz3<void>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool decompressSz3<float>(float *dest, const buffer &compressed, const Index dim[3]);
extern template bool decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3]);
extern template bool decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3]);
extern template bool decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3]);
#endif
#endif

} // namespace detail

} // namespace vistle
#endif
