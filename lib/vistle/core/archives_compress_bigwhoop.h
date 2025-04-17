#ifndef VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H
#define VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H

#include <iostream>

#include <type_traits>

#include "archives_compression_settings.h"

namespace vistle {
namespace detail {

template<typename T>
size_t compressBigWhoop(T *src, const Index dim[3], char *compressed, const CompressionSettings &config)
{
    std::cerr << "Big Whoop only supports floating point compression!" << std::endl;
    return 0;
}

template<>
size_t V_COREEXPORT compressBigWhoop<float>(float *src, const Index dim[3], char *compressed,
                                            const CompressionSettings &config);
template<>
size_t V_COREEXPORT compressBigWhoop<double>(double *src, const Index dim[3], char *compressed,
                                             const CompressionSettings &config);


template<typename T>
bool decompressBigWhoop(T *dest, char *compressed, uint8_t layer)
{
    return false;
}

template<>
bool V_COREEXPORT decompressBigWhoop<float>(float *dest, char *compressed, uint8_t layer);

template<>
bool V_COREEXPORT decompressBigWhoop<double>(double *dest, char *compressed, uint8_t layer);

} // namespace detail
} // namespace vistle

#endif
