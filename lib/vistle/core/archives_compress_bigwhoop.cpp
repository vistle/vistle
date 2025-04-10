#include <iostream>

#include <bwc.h>

#include "index.h"
#include "archives_compress_bigwhoop.h"

// TODO: find out if BigWhoop also works with int data

namespace vistle {
namespace detail {

template<typename T>
size_t compressBigWhoop(T *toCompress, const Index dim[3], T *compressed, const CompressionSettings &config)
{
    std::cerr << "BigWhoop only supports floating point compression" << std::endl;
    return 0;
}

template<>
size_t compressBigWhoop<float>(float *toCompress, const Index dim[3], float *compressed,
                               const CompressionSettings &config)
{
    bwc_codec *coder = bwc_alloc_coder(dim[0], dim[1], dim[2], 1, config.p_bigWhoop_nPar, bwc_precision_single);
    bwc_stream *stream = bwc_init_stream(toCompress, compressed, comp);

    bwc_create_compression(coder, stream, const_cast<char *>(config.p_bigWhoop_rate));
    size_t compressed_size = bwc_compress(coder, stream);

    bwc_free_codec(coder);

    return compressed_size;
}

template<>
size_t compressBigWhoop<double>(double *toCompress, const Index dim[3], double *compressed,
                                const CompressionSettings &config)
{
    bwc_codec *coder = bwc_alloc_coder(dim[0], dim[1], dim[2], 1, config.p_bigWhoop_nPar, bwc_precision_double);
    bwc_stream *stream = bwc_init_stream(toCompress, compressed, comp);

    bwc_create_compression(coder, stream, const_cast<char *>(config.p_bigWhoop_rate));
    size_t compressed_size = bwc_compress(coder, stream);

    bwc_free_codec(coder);

    return compressed_size;
}

template<typename T, typename std::enable_if<!std::is_floating_point<T>::value, int>::type>
void decompressBigWhoop(T *toDecompress, const Index dim[3], T *decompressed, uint8_t layer)
{
    std::cerr << "BigWhoop only supports floating point decompression" << std::endl;
}

template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type>
void decompressBigWhoop(T *toDecompress, const Index dim[3], T *decompressed, uint8_t layer)
{
    bwc_codec *decoder = bwc_alloc_decoder();
    bwc_stream *stream = bwc_init_stream(toDecompress, decompressed, decomp);

    bwc_create_decompression(decoder, stream, layer);
    bwc_decompress(decoder, stream);
    bwc_free_codec(decoder);
}

template size_t compressBigWhoop<void>(void *toCompress, const Index dim[3], void *compressed,
                                       const CompressionSettings &config);
template size_t compressBigWhoop<float>(float *toCompress, const Index dim[3], float *compressed,
                                        const CompressionSettings &config);
template size_t compressBigWhoop<double>(double *toCompress, const Index dim[3], double *compressed,
                                         const CompressionSettings &config);
template size_t compressBigWhoop<int32_t>(int32_t *toCompress, const Index dim[3], int32_t *compressed,
                                          const CompressionSettings &config);
template size_t compressBigWhoop<int64_t>(int64_t *toCompress, const Index dim[3], int64_t *compressed,
                                          const CompressionSettings &config);

template void vistle::detail::decompressBigWhoop<void>(void *toDecompress, const Index dim[3], void *decompressed,
                                                       uint8_t layer);
template void vistle::detail::decompressBigWhoop<float>(float *toDecompress, const Index dim[3], float *decompressed,
                                                        uint8_t layer);
template void vistle::detail::decompressBigWhoop<double>(double *toDecompress, const Index dim[3], double *decompressed,
                                                         uint8_t layer);
template void vistle::detail::decompressBigWhoop<int32_t>(int32_t *toDecompress, const Index dim[3],
                                                          int32_t *decompressed, uint8_t layer);
template void vistle::detail::decompressBigWhoop<int64_t>(int64_t *toDecompress, const Index dim[3],
                                                          int64_t *decompressed, uint8_t layer);


} // namespace detail
} // namespace vistle
