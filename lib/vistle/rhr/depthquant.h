#ifndef VISTLE_RHR_DEPTHQUANT_H
#define VISTLE_RHR_DEPTHQUANT_H

/**\file
 * \brief Depth quantization for remote hybrid rendering
 * 
 * \author Martin Aumüller <aumueller@hlrs.de>
 * \author (c) 2013 HLRS
 */

#include "export.h"

#include <stdint.h>
#include <cstdlib>

namespace vistle {

enum DepthFormat {
    DepthInteger,
    DepthFloat,
    DepthRGBA,
};

//! quantized depth data for a single 4x4 pixel tile
template<int Precision, int BitsPerPixel, int ScaleBits = 0>
struct DepthQuantizeBits {
    enum {
        precision = Precision,

        edge = 4, // only 4 works
        scale_bits = ScaleBits,
        bits_per_pixel = BitsPerPixel,
        num_bytes = (BitsPerPixel * edge * edge) / 8
    };

    uint8_t bits[num_bytes]; //!< per-pixel interpolation weights
};

typedef DepthQuantizeBits<16, 4, 0> DepthQuantizeBits16;
typedef DepthQuantizeBits<24, 3, 4> DepthQuantizeBits24;

//! min/max depth for a single 4x4 pixel tile (when storing depths and interpolation bits separately)
template<int Precision>
struct DepthQuantizeMinMaxDepth {
    uint8_t depth[2][Precision / 8]; //!< minimum and maximum depth data
};

typedef DepthQuantizeMinMaxDepth<16> MinMaxDepth16;
typedef DepthQuantizeMinMaxDepth<24> MinMaxDepth24;

//! quantized depth data for a single 4x4 pixel tile
template<int Precision, int BitsPerPixel, int ScaleBits = 0>
struct DepthQuantize: public DepthQuantizeBits<Precision, BitsPerPixel, ScaleBits> {
    uint8_t depth[2][Precision / 8]; //!< minimum and maximum depth data
};

typedef DepthQuantize<16, 4, 0> DepthQuantize16;
typedef DepthQuantize<24, 3, 4> DepthQuantize24;

//! set depth bits for a single pixel
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline void
    setdepth(DepthQuantize &q, int idx, uint32_t value)
{
    for (unsigned i = 3; i > 0; --i) {
        if (i <= sizeof(q.depth[idx]))
            q.depth[idx][i - 1] = value & 0xff;
        value >>= 8;
    }
}

//! retrieve depth bits for a single pixel
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    uint32_t
    getdepth(const DepthQuantize &q, int idx);

//! zero depth bits for a 4x4 tile
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    void
    dq_clearbits(DepthQuantize &q)
{
    for (unsigned i = 0; i < sizeof(q.bits); ++i) {
        q.bits[i] = 0;
    }
}

//! set a single bit in DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline void
    dq_setbit(DepthQuantize &q, int bit, unsigned value = 1)
{
    q.bits[bit >> 3] |= (value & 1) << (bit & 7);
}

//! clear a single bit in DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    void
    dq_clearbit(DepthQuantize &q, int bit)
{
    q.bits[bit >> 3] &= ~(1 << (bit & 7));
}

//! retrieve a single bit from DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline unsigned
    dq_getbit(const DepthQuantize &q, int bit)
{
    return (q.bits[bit >> 3] >> (bit & 7)) & 1;
}

//! set multiple bits in DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline void
    dq_setbits(DepthQuantize &q, int bit, int nbits, unsigned value, int block = 1)
{
#if 0
   for (int i=bit; i<bit+nbits; ++i) {
      dq_setbit(q, i, value&1);
      value >>= 1;
   }
#else
    const uint32_t mask = (1 << block) - 1;
    for (int i = bit + nbits - 1; i >= bit; i -= block) {
        q.bits[i >> 3] |= (value & mask) << (i & 7);
        value >>= block;
    }
#endif
}

//! set all bits in DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline void
    dq_setbits(DepthQuantize &q, uint64_t bits)
{
    for (int i = 0; i < DepthQuantize::num_bytes; ++i) {
        //for (int i = DepthQuantize::num_bytes; i>=0; --i) {
        q.bits[i] = bits & 0xff;
        bits >>= 8;
    }
}

//! get multiple bits from DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline unsigned
    dq_getbits(const DepthQuantize &q, int bit, int nbits)
{
    unsigned v = 0;
    for (int i = bit + nbits - 1; i >= bit; --i) {
        v <<= 1;
        v |= dq_getbit(q, i);
    }
    return v;
}

//! get all bits from DepthQuantize struct
template<class DepthQuantize>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint64_t
    dq_getbits(const DepthQuantize &q)
{
    uint64_t bits = q.bits[DepthQuantize::num_bytes - 1];
    for (int i = DepthQuantize::num_bytes - 2; i >= 0; --i) {
        bits <<= 8;
        bits |= q.bits[i];
    }
    return bits;
}

template<DepthFormat format, int bpp>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth(const unsigned char *img, int x, int y, int w, int h);

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthFloat, 4>(const unsigned char *img, int x, int y, int w, int h)
{
    const float df = ((float *)img)[y * w + x];
    uint32_t di = df * 0x00ffffffU;
    if (di > 0x00ffffffU)
        di = 0x00ffffffU;
    return di;
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthRGBA, 4>(const unsigned char *img, int x, int y, int w, int h)
{
    const unsigned char *dp = &img[(y * w + x) * 4];
    return ((dp[2] * 256 + dp[1]) * 256 + dp[0]);
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthInteger, 1>(const unsigned char *img, int x, int y, int w, int h)
{
    const unsigned char *dp = &img[(y * w + x) * 1];
    return *dp * 0x0010101U;
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthInteger, 2>(const unsigned char *img, int x, int y, int w, int h)
{
    const unsigned char *dp = &img[(y * w + x) * 2];
    return (dp[1] * 256 + dp[0]) * 256 + dp[1];
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthInteger, 3>(const unsigned char *img, int x, int y, int w, int h)
{
    const unsigned char *dp = &img[(y * w + x) * 3];
    return ((dp[3] * 256 + dp[2]) * 256 + dp[1]);
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    get_depth<DepthInteger, 4>(const unsigned char *img, int x, int y, int w, int h)
{
    const unsigned char *dp = &img[(y * w + x) * 4];
    return ((dp[3] * 256 + dp[2]) * 256 + dp[1]);
}

//! transform depth buffer into quantized values on 4x4 pixel tiles
void V_RHREXPORT depthquant(char *quantbuf, const char *zbuf, DepthFormat format, int depthps, int x, int y, int width,
                            int height, int stride = -1);
void V_RHREXPORT depthquant_planar(char *quantbuf, const char *zbuf, DepthFormat format, int depthps, int x, int y,
                                   int width, int height, int stride = -1);

//! return size required by depthquant for quantized image
size_t V_RHREXPORT depthquant_size(DepthFormat format, int depthps, int width, int height);

//! reverse transformation done by depthquant
void V_RHREXPORT depthdequant(char *zbuf, const char *quantbuf, DepthFormat format, int depthps, int tx, int dy,
                              int width, int height, int stride = -1);
void V_RHREXPORT depthdequant_planar(char *zbuf, const char *quantbuf, DepthFormat format, int depthps, int tx, int dy,
                                     int width, int height, int stride = -1);

} // namespace vistle
#endif
