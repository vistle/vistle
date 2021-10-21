#ifndef RHR_COMPDECOMP_H
#define RHR_COMPDECOMP_H

#include <vistle/core/message.h>
#include <vistle/util/enum.h>
#include <vistle/util/buffer.h>
#include <vector>
#include "export.h"

namespace vistle {

struct CompressionParameters {
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ColorCodec, (Raw)(PredictRGB)(PredictRGBA)(Jpeg_YUV411)(Jpeg_YUV444))

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(
        DepthCodec, (DepthRaw)(DepthPredict)(DepthPredictPlanar)(DepthQuant)(DepthQuantPlanar)(DepthZfp))

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ZfpMode, (ZfpFixedRate)(ZfpPrecision)(ZfpAccuracy))

    struct DepthCompressionParameters {
        bool depthFloat = true; //!< whether depth should be retrieved as floating point
        int depthPrecision = 24; //!< depth buffer read-back precision (bits) for integer formats
        DepthCodec depthCodec = DepthRaw;
        ZfpMode depthZfpMode = ZfpPrecision;
        message::CompressionMode depthCompress = message::CompressionNone;
    };

    struct RgbaCompressionParameters {
        ColorCodec rgbaCodec = Raw;
        message::CompressionMode rgbaCompress = message::CompressionNone;
    };

    bool isDepth = false;
    DepthCompressionParameters depth;
    RgbaCompressionParameters rgba;

    CompressionParameters(const RgbaCompressionParameters &rgba): isDepth(false), rgba(rgba) {}

    CompressionParameters(const DepthCompressionParameters &depth): isDepth(true), depth(depth) {}

    CompressionParameters() = default;
};

using RgbaCompressionParameters = CompressionParameters::RgbaCompressionParameters;
buffer V_RHREXPORT compressRgba(const unsigned char *rgba, int x, int y, int w, int h, int stride,
                                RgbaCompressionParameters &param);

using DepthCompressionParameters = CompressionParameters::DepthCompressionParameters;
buffer V_RHREXPORT compressDepth(const float *depth, int x, int y, int w, int h, int stride,
                                 DepthCompressionParameters &param);

bool V_RHREXPORT decompressTile(char *dest, const buffer &input, CompressionParameters param, int x, int y, int w,
                                int h, int stride);

} // namespace vistle
#endif
