#ifndef VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H
#define VISTLE_CORE_ARCHIVES_COMPRESS_BIGWHOOP_H

#include <iostream>

#include <type_traits>

#include "archives_compression_settings.h"

//TODO: make function parameter order match the other compression methods
namespace vistle {
namespace detail {

template<typename T>
size_t compressBigWhoop(T *toCompress, const Index dim[3], T *compressed, const CompressionSettings &config)
{
    std::cerr << "Big Whoop only supports floating point compression!" << std::endl;
    return 0;
}

template<>
size_t V_COREEXPORT compressBigWhoop<float>(float *toCompress, const Index dim[3], float *compressed,
                                            const CompressionSettings &config);
template<>
size_t V_COREEXPORT compressBigWhoop<double>(double *toCompress, const Index dim[3], double *compressed,
                                             const CompressionSettings &config);


template<typename T>
void decompressBigWhoop(T *toDecompress, T *decompressed, uint8_t layer)
{
    std::cerr << "Big Whoop only supports floating point decompression!" << std::endl;
}

template<>
void V_COREEXPORT decompressBigWhoop<float>(float *toDecompress, float *decompressed, uint8_t layer);

template<>
void V_COREEXPORT decompressBigWhoop<double>(double *toDecompress, double *decompressed, uint8_t layer);

} // namespace detail
} // namespace vistle

#endif
