#ifndef RHR_COMPDECOMP_H
#define RHR_COMPDECOMP_H

#include <core/message.h>
#include <util/enum.h>
#include <util/buffer.h>
#include <vector>
#include "export.h"

namespace vistle {

struct CompressionParameters {

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ColorCodec,
                                        (Raw)
                                        (Jpeg_YUV411)
                                        (Jpeg_YUV444)
                                        )

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(DepthCodec, (DepthRaw)(DepthPredict)(DepthQuant)(DepthZfp))

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ZfpMode, (ZfpFixedRate)(ZfpPrecision)(ZfpAccuracy))

    struct DepthCompressionParameters {
        bool depthFloat = true; //!< whether depth should be retrieved as floating point
        int depthPrecision = 24; //!< depth buffer read-back precision (bits) for integer formats
#if 0
        bool depthZfp = false; //!< whether depth should be compressed with floating point compressor zfp
        bool depthQuant = false; //!< whether depth should be sent quantized
#endif
        DepthCodec depthCodec = DepthRaw;
        ZfpMode depthZfpMode = ZfpPrecision;
        message::CompressionMode depthCompress = message::CompressionNone;
    };

    struct RgbaCompressionParameters {

        bool rgbaJpeg = false;
        bool rgbaChromaSubsamp = false;
        message::CompressionMode rgbaCompress = message::CompressionNone;
    };

    bool isDepth = false;
    DepthCompressionParameters depth;
    RgbaCompressionParameters rgba;

    CompressionParameters(const RgbaCompressionParameters &rgba)
        : isDepth(false)
        , rgba(rgba)
    {}

    CompressionParameters(const DepthCompressionParameters &depth)
        : isDepth(true)
        , depth(depth)
    {}

    CompressionParameters() = default;
};

using RgbaCompressionParameters = CompressionParameters::RgbaCompressionParameters;
buffer V_RHREXPORT compressRgba(const unsigned char *rgba,
                                int x, int y, int w, int h, int stride,
                                RgbaCompressionParameters &param);

using DepthCompressionParameters = CompressionParameters::DepthCompressionParameters;
buffer V_RHREXPORT compressDepth(const float *depth,
                                int x, int y, int w, int h, int stride,
                                DepthCompressionParameters &param);

bool V_RHREXPORT decompressTile(char *dest, const buffer &input,
                    CompressionParameters param,
                    int x, int y, int w, int h, int stride);

}
#endif
