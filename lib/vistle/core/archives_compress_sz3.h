#ifndef VISTLE_CORE_ARCHIVES_COMPRESS_SZ3_H
#define VISTLE_CORE_ARCHIVES_COMPRESS_SZ3_H

#include <vistle/util/buffer.h>
#include "index.h"
#include "export.h"
#include "archives_compression_settings.h"

namespace vistle {

namespace detail {

template<typename T>
char *compressSz3(size_t &compressedSize, const T *src, const Index dim[3], const CompressionSettings &conf);

template<typename T>
bool decompressSz3(T *dest, const buffer &compressed, const Index dim[3]);

template<>
char V_COREEXPORT *compressSz3<void>(size_t &compressedSize, const void *src, const Index dim[3],
                                     const CompressionSettings &conf);
extern template char V_COREEXPORT *compressSz3<float>(size_t &compressedSize, const float *src, const Index dim[3],
                                                      const CompressionSettings &conf);
extern template char V_COREEXPORT *compressSz3<double>(size_t &compressedSize, const double *src, const Index dim[3],
                                                       const CompressionSettings &conf);
extern template char V_COREEXPORT *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const Index dim[3],
                                                        const CompressionSettings &conf);
extern template char V_COREEXPORT *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const Index dim[3],
                                                        const CompressionSettings &conf);

template<>
bool V_COREEXPORT decompressSz3<void>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<float>(float *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3]);

} // namespace detail

} // namespace vistle
#endif
