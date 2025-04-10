#ifndef VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H
#define VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H

#include <type_traits>

#include "archives_compression_settings.h"

namespace vistle {
namespace detail {

template<typename T>
size_t V_COREEXPORT compressBigWhoop(T *toCompress, const Index dim[3], T *compressed,
                                     const CompressionSettings &config);
template<>
size_t V_COREEXPORT compressBigWhoop<float>(float *toCompress, const Index dim[3], float *compressed,
                                            const CompressionSettings &config);
template<>
size_t V_COREEXPORT compressBigWhoop<double>(double *toCompress, const Index dim[3], double *compressed,
                                             const CompressionSettings &config);


template<typename T, typename std::enable_if<!std::is_floating_point<T>::value, int>::type = 0>
void V_COREEXPORT decompressBigWhoop(T *toDecompress, const Index dim[3], T *decompressed, uint8_t layer);

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
void V_COREEXPORT decompressBigWhoop(T *toDecompress, const Index dim[3], T *decompressed, uint8_t layer);

} // namespace detail
} // namespace vistle

#endif
