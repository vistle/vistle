#ifndef RHR_COMPDECOMP_H
#define RHR_COMPDECOMP_H

#include <core/message.h>
#include <util/enum.h>
#include <vector>
#include "export.h"

namespace vistle {


struct DepthCompressionParameters {
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ZfpMode, (ZfpFixedRate)(ZfpPrecision)(ZfpAccuracy))
    bool depthFloat = true; //!< whether depth should be retrieved as floating point
    int depthPrecision = 24; //!< depth buffer read-back precision (bits) for integer formats
    bool depthZfp = false; //!< whether depth should be compressed with floating point compressor zfp
    bool depthQuant = false; //!< whether depth should be sent quantized
    ZfpMode depthZfpMode = ZfpPrecision;
    message::CompressionMode depthCompress = message::CompressionNone;
};

struct RgbaCompressionParameters {

    bool rgbaJpeg = false;
    bool rgbaChromaSubsamp = false;
    message::CompressionMode rgbaCompress = message::CompressionNone;
};

struct CompressionParameters {

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(ColorCodec,
                                        (Raw)
                                        (Jpeg_YUV411)
                                        (Jpeg_YUV444)
                                        )

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


std::vector<char> V_RHREXPORT compressRgba(const unsigned char *rgba,
                                int x, int y, int w, int h, int stride,
                                RgbaCompressionParameters &param);

std::vector<char> V_RHREXPORT compressDepth(const float *depth,
                                int x, int y, int w, int h, int stride,
                                DepthCompressionParameters &param);

bool V_RHREXPORT decompressTile(char *dest, const std::vector<char> &input,
                    CompressionParameters param,
                    int x, int y, int w, int h, int stride);

}
#endif
