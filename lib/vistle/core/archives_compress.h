#ifndef VISTLE_ARCHIVES_COMPRESS_H
#define VISTLE_ARCHIVES_COMPRESS_H

#ifdef HAVE_ZFP
#include <zfp.h>
#endif


#include <vistle/util/buffer.h>
#include "archives_config.h"

namespace vistle {

#ifdef HAVE_ZFP
namespace detail {

struct ZfpParameters {
    FieldCompressionZfpMode mode = ZfpFixedRate;
    double rate = 8.;
    int precision = 20;
    double accuracy = 1e-20;
};

template<zfp_type type>
bool compressZfp(buffer &compressed, const void *src, const Index dim[3], Index typeSize, const ZfpParameters &param);
template<zfp_type type>
bool decompressZfp(void *dest, const buffer &compressed, const Index dim[3]);


template<>
bool V_COREEXPORT decompressZfp<zfp_type_none>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int32>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int64>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_float>(void *dest, const buffer &compressed,
                                                                const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_double>(void *dest, const buffer &compressed,
                                                                 const Index dim[3]);

template<>
bool V_COREEXPORT compressZfp<zfp_type_none>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                             const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int32>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int64>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_float>(buffer &compressed, const void *src, const Index dim[3],
                                                              Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_double>(buffer &compressed, const void *src, const Index dim[3],
                                                               Index typeSize, const ZfpParameters &param);


extern template bool V_COREEXPORT decompressZfp<zfp_type_none>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int32>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_int64>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_float>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressZfp<zfp_type_double>(void *dest, const buffer &compressed, const Index dim[3]);

extern template bool V_COREEXPORT compressZfp<zfp_type_none>(buffer &compressed, const void *src, const Index dim[3],
                                                const Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int32>(buffer &compressed, const void *src, const Index dim[3],
                                                 const Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_int64>(buffer &compressed, const void *src, const Index dim[3],
                                                 const Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_float>(buffer &compressed, const void *src, const Index dim[3],
                                                 const Index typeSize, const ZfpParameters &param);
extern template bool V_COREEXPORT compressZfp<zfp_type_double>(buffer &compressed, const void *src, const Index dim[3],
                                                  const Index typeSize, const ZfpParameters &param);

} // namespace detail
#endif

} // namespace vistle
#endif
