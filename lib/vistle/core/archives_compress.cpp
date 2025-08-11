/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_oarchive.cpp:
// binary_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include "archives_config.h"
#include "archives_impl.h"
#include "archives.h"
#include "archives_compress.h"

//#define COMP_DEBUG

namespace vistle {

namespace detail {

template<zfp_type type>
bool decompressZfp(void *dest, const buffer &compressed, const Index dim[3])
{
    bool ok = true;
    bitstream *stream = stream_open(const_cast<char *>(compressed.data()), compressed.size());
    zfp_stream *zfp = zfp_stream_open(stream);
    zfp_stream_rewind(zfp);
    zfp_field *field = zfp_field_3d(dest, type, dim[0], dim[1], dim[2]);
    if (!zfp_read_header(zfp, field, ZFP_HEADER_FULL)) {
        std::cerr << "decompressZfp: reading zfp compression parameters failed" << std::endl;
    }
    if (field->type != type) {
        std::cerr << "decompressZfp: zfp type not compatible" << std::endl;
    }
    if (field->nx != dim[0] || field->ny != dim[1] || field->nz != dim[2]) {
        std::cerr << "decompressZfp: zfp size mismatch: " << field->nx << "x" << field->ny << "x" << field->nz
                  << " != " << dim[0] << "x" << dim[1] << "x" << dim[2] << std::endl;
    }
    zfp_field_set_pointer(field, dest);
    if (!zfp_decompress(zfp, field)) {
        std::cerr << "decompressZfp: zfp decompression failed" << std::endl;
        ok = false;
    }
    zfp_stream_close(zfp);
    zfp_field_free(field);
    stream_close(stream);
    return ok;
}

template<>
bool decompressZfp<zfp_type_none>(void *dest, const buffer &compressed, const Index dim[3])
{
    return false;
}

template bool decompressZfp<zfp_type_int32>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_int64>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_float>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_double>(void *dest, const buffer &compressed, const Index dim[3]);

template<zfp_type type>
bool compressZfp(buffer &compressed, const void *src, const Index dim[3], const Index typeSize,
                 const ZfpParameters &param)
{
    int ndims = 1;
    size_t sz = dim[0];
    if (dim[1] != 0) {
        ndims = 2;
        sz *= dim[1];
        if (dim[2] != 0) {
            ndims = 3;
            sz *= dim[2];
        }
    }

    if (sz < 1000) {
#ifdef COMP_DEBUG
        std::cerr << "compressZfp: not compressing - fewer than 1000 elements" << std::endl;
#endif
        return false;
    }

    zfp_stream *zfp = zfp_stream_open(nullptr);
    switch (param.mode) {
    case ZfpAccuracy:
        zfp_stream_set_accuracy(zfp, param.accuracy);
        break;
    case ZfpPrecision:
        zfp_stream_set_precision(zfp, param.precision);
        break;
    case ZfpFixedRate:
        zfp_stream_set_rate(zfp, param.rate, type, ndims, 0);
        break;
    }

    zfp_field *field = zfp_field_3d(const_cast<void *>(src), type, dim[0], dim[1], dim[2]);
    size_t bufsize = zfp_stream_maximum_size(zfp, field);
    compressed.resize(bufsize);
    bitstream *stream = stream_open(compressed.data(), bufsize);
    zfp_stream_set_bit_stream(zfp, stream);

    zfp_stream_rewind(zfp);
    size_t header = zfp_write_header(zfp, field, ZFP_HEADER_FULL);
#ifdef COMP_DEBUG
    std::cerr << "compressZfp: wrote " << header << " header bytes" << std::endl;
#else
    (void)header;
#endif
    size_t zfpsize = zfp_compress(zfp, field);
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);
    if (zfpsize == 0) {
        std::cerr << "compressZfp: zfp compression failed" << std::endl;
        return false;
    }
    compressed.resize(zfpsize);
#ifdef COMP_DEBUG
    std::cerr << "compressZfp: compressed " << dim[0] << "x" << dim[1] << "x" << dim[2] << " elements, size "
              << sz * typeSize << " to " << zfpsize << " bytes" << std::endl;
#endif
    return true;
}

template<>
bool compressZfp<zfp_type_none>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                const ZfpParameters &param)
{
#ifdef COMP_DEBUG
    std::cerr << "compressZfp: type not capable of compression" << std::endl;
#endif
    return false;
}
template bool compressZfp<zfp_type_int32>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_int64>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_float>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_double>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                           const ZfpParameters &param);

} // namespace detail

} // namespace vistle
