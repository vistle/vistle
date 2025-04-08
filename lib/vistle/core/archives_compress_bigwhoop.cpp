#include <bwc.h>

#include "index.h"
#include "archives_compress_bigwhoop.h"

namespace vistle {
namespace detail {

// BigWhoop seems to only support floating point compression
template<typename T>
size_t compressBigWhoop(T *toCompress, const Index dim[3], T *compressed, const BigWhoopParameters &parameters)
{
    return 0;
}

template<>
size_t compressBigWhoop(float *toCompress, const Index dim[3], float *compressed, const BigWhoopParameters &parameters)
{
    bwc_codec *coder = bwc_alloc_coder(dim[0], dim[1], dim[2], 1, parameters.nPar, bwc_precision_single);
    bwc_stream *stream = bwc_init_stream(toCompress, compressed, comp);

    bwc_create_compression(coder, stream, parameters.rate);
    size_t compressed_size = bwc_compress(coder, stream);

    bwc_free_codec(coder);

    return compressed_size;
}

template<>
size_t compressBigWhoop(double *toCompress, const Index dim[3], double *compressed,
                        const BigWhoopParameters &parameters)
{
    bwc_codec *coder = bwc_alloc_coder(dim[0], dim[1], dim[2], 1, parameters.nPar, bwc_precision_double);
    bwc_stream *stream = bwc_init_stream(toCompress, compressed, comp);

    bwc_create_compression(coder, stream, parameters.rate);
    size_t compressed_size = bwc_compress(coder, stream);

    bwc_free_codec(coder);

    return compressed_size;
}
} // namespace detail
} // namespace vistle
