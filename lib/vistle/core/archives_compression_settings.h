#ifndef VISTLE_CORE_ARCHIVES_COMPRESSION_SETTINGS_H
#define VISTLE_CORE_ARCHIVES_COMPRESSION_SETTINGS_H

#include <vistle/util/enum.h>

#include "export.h"

namespace vistle {

DEFINE_ENUM_WITH_STRING_CONVERSIONS(FieldCompressionMode, (Uncompressed)(Predict)(Zfp)(SZ)(BigWhoop))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(FieldCompressionZfpMode, (ZfpFixedRate)(ZfpPrecision)(ZfpAccuracy))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(FieldCompressionSzAlgo, (SzInterpLorenzo)(SzInterp)(SzLorenzoReg))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(FieldCompressionSzError, (SzRel)(SzAbs)(SzAbsAndRel)(SzAbsOrRel)(SzPsnr)(SzL2))

struct CompressionSettings {
    static constexpr const char *p_mode = "field_compression";
    FieldCompressionMode mode = Uncompressed;

    static constexpr const char *p_zfpMode = "zfp_mode";
    FieldCompressionZfpMode zfpMode = ZfpFixedRate;
    static constexpr const char *p_zfpRate = "zfp_rate";
    double zfpRate = 16.;
    static constexpr const char *p_zfpPrecision = "zfp_precision";
    int zfpPrecision = 8;
    static constexpr const char *p_zfpAccuracy = "zfp_accuracy";
    double zfpAccuracy = 1e-20;

    static constexpr const char *p_szAlgo = "sz_algo";
    FieldCompressionSzAlgo szAlgo = SzInterpLorenzo;
    static constexpr const char *p_szError = "sz_error_control";
    FieldCompressionSzError szError = SzRel;
    static constexpr const char *p_szAbsError = "sz_abs_error";
    double szAbsError = 1e-3;
    static constexpr const char *p_szRelError = "sz_rel_error";
    double szRelError = 1e-4;
    static constexpr const char *p_szPsnrError = "sz_psnr_error";
    double szPsnrError = 80;
    static constexpr const char *p_szL2Error = "sz_l2_error";
    double szL2Error = 1e-1;

    uint8_t p_bigWhoop_nPar = 1;
    const char *p_bigWhoop_rate = "32";
};

} // namespace vistle

#endif
