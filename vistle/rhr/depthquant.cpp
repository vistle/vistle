#include "depthquant.h"
#include "predict.h"
#include <vistle/util/buffer.h>
#include <algorithm>
#include <iostream>
#include <cmath>

//#define TIMING

#ifdef TIMING
#include "stopwatch.h"
#endif

#include <cassert>

typedef unsigned char uchar;

namespace vistle {

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    getdepth<DepthQuantize16>(const DepthQuantize16 &q, int idx)
{
    const uint32_t value = (q.depth[idx][0] << 16) | (q.depth[idx][1] << 8) | q.depth[idx][0];
    return value;
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    getdepth<DepthQuantize24>(const DepthQuantize24 &q, int idx)
{
    const uint32_t value = (q.depth[idx][0] << 16) | (q.depth[idx][1] << 8) | q.depth[idx][2];
    return value;
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    getdepth<MinMaxDepth16>(const MinMaxDepth16 &q, int idx)
{
    const uint32_t value = (q.depth[idx][0] << 16) | (q.depth[idx][1] << 8) | q.depth[idx][0];
    return value;
}

template<>
#ifdef __CUDACC__
__device__ __host__
#endif
    inline uint32_t
    getdepth<MinMaxDepth24>(const MinMaxDepth24 &q, int idx)
{
    const uint32_t value = (q.depth[idx][0] << 16) | (q.depth[idx][1] << 8) | q.depth[idx][2];
    return value;
}

size_t depthquant_size(DepthFormat format, int depthps, int width, int height)
{
    size_t edge = 1, tile = depthps;
    if (depthps <= 2) {
        edge = DepthQuantize16::edge;
        tile = sizeof(DepthQuantize16);
    } else {
        edge = DepthQuantize24::edge;
        tile = sizeof(DepthQuantize24);
    }
    const size_t ntiles = ((width + edge - 1) / edge) * ((height + edge - 1) / edge);

    return tile * ntiles;
}

template<typename ZType>
inline void set_z(ZType &z, uint32_t val);

template<>
inline void set_z<uint16_t>(uint16_t &z, uint32_t val)
{
    z = val >> 8;
}

template<>
inline void set_z<uint32_t>(uint32_t &z, uint32_t val)
{
    z = val << 8;
}

template<>
inline void set_z<float>(float &z, uint32_t val)
{
    z = val / (float)0xffffff;
}

template<class Quant, class MinMaxDepth, typename ZType>
static void depthdequant_t(ZType *zbuf, const Quant *quantbuf, const MinMaxDepth *mmdepth, DepthFormat format, int dx,
                           int dy, int width, int height, int stride)
{
    const int edge = Quant::edge;
    const int size = edge * edge;
    const int quantbits = Quant::bits_per_pixel;
    const int scalebits = Quant::scale_bits;
    const uint32_t scalemask = (1U << scalebits) - 1U;
    const uint32_t depthmask = ~scalemask;
    const uint32_t mask = (1U << quantbits) - 1U;
    const uint32_t Far = 0x00ffffff;
    const uint32_t Valid = Quant::precision == 24 ? (Far & ~((1U << scalebits) - 1)) : 0x00ffff00;
    const uint32_t Next = Far - Valid + 1;
    const uint32_t qm2 = (1U << quantbits) / 2;
    const uint64_t AllFar = Quant::num_bytes == 8 ? ~uint64_t(0) : ((1ULL << (Quant::num_bytes * 8)) - 1ULL);

    if (scalebits == 0) {
        assert(scalemask == 0);
    }

    if (scalebits == 0) {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
        for (int y = 0; y < height; y += edge) {
            //std::cerr << "tid: " << omp_get_thread_num() << std::endl;
            for (int x = 0; x < width; x += edge) {
                const Quant &quant = quantbuf[(y / edge) * ((width + edge - 1) / edge) + x / edge];
                const MinMaxDepth &mmd = mmdepth[(y / edge) * ((width + edge - 1) / edge) + x / edge];
                uint32_t d1 = getdepth(mmd, 0), d2 = getdepth(mmd, 1);
                uint64_t bits = dq_getbits(quant);

                if (d1 < d2) {
                    d2 += Next - 1;
                    // full interpolation range is mapped from d1..d2
                    const float range = d2 - d1;
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        const uint32_t q = bits & mask;
                        bits >>= quantbits;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        uint32_t zoff = (q * range) / mask;
                        uint32_t zz = d1 + zoff;
                        set_z(zbuf[(yy + dy) * stride + dx + xx], zz);
                    }
                } else if (bits == AllFar) {
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        set_z(zbuf[(yy + dy) * stride + dx + xx], Far);
                    }
                } else if (bits == 0) {
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        set_z(zbuf[(yy + dy) * stride + dx + xx], d1);
                    }
                } else {
                    d1 += Next - 1;
                    // max quantized interpolation value means maximum possible framebuffer depth
                    const float range = d1 - d2;
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        const uint32_t q = bits & mask;
                        bits >>= quantbits;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        uint32_t zz = Far;
                        if (q != mask) {
                            uint32_t zoff = q * range / (mask - 1);
                            zz = d2 + zoff;
                        }
                        set_z(zbuf[(yy + dy) * stride + dx + xx], zz);
                    }
                }
            }
        }
    } else {
#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (int y = 0; y < height; y += edge) {
            for (int x = 0; x < width; x += edge) {
                const Quant &quant = quantbuf[(y / edge) * ((width + edge - 1) / edge) + x / edge];
                const MinMaxDepth &mmd = mmdepth[(y / edge) * ((width + edge - 1) / edge) + x / edge];
                uint32_t d1 = getdepth(mmd, 0), d2 = getdepth(mmd, 1);
                const int s1 = 1 + (d1 & scalemask), s2 = 1 + (d2 & scalemask);
                d1 &= depthmask;
                d2 &= depthmask;
                uint64_t bits = dq_getbits(quant);

                if (d1 < d2) {
                    d2 += Next - 1;
                    d2 |= scalemask;
                    const int lowerscale = s1;
                    const int upperscale = s2;
                    // full interpolation range is mapped from d1..d2
                    const float range = d2 - d1;
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        const uint32_t q = bits & mask;
                        bits >>= quantbits;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        uint32_t zz;
                        if (q < qm2) {
                            uint32_t zoff = (q * range / lowerscale) / mask;
                            zz = d1 + zoff;
                        } else {
                            int qq = mask - q;
                            uint32_t zoff = (qq * range / upperscale) / mask;
                            zz = d2 - zoff;
                        }

                        set_z(zbuf[(yy + dy) * stride + dx + xx], zz);
                    }
                } else if (bits == AllFar || bits == 0) {
                    const uint32_t z = bits == AllFar ? Far : d1;
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        set_z(zbuf[(yy + dy) * stride + dx + xx], z);
                    }
                } else {
                    d1 += Next - 1;
                    d1 |= scalemask;
                    const int lowerscale = s2;
                    const int upperscale = s1;
                    // max quantized interpolation value means maximum possible framebuffer depth
                    const float range = d1 - d2;
                    for (int i = 0; i < size; ++i) {
                        const int xx = x + i % edge;
                        const int yy = y + i / edge;
                        const uint32_t q = bits & mask;
                        bits >>= quantbits;
                        if (xx >= width)
                            continue;
                        if (yy >= height)
                            continue;
                        uint32_t zz;
                        if (q < qm2) {
                            uint32_t zoff = (q * range / lowerscale) / mask;
                            zz = d2 + zoff;
                        } else if (q < mask) {
                            int qq = mask - 1 - q;
                            uint32_t zoff = (qq * range / upperscale) / (mask - 2);
                            zz = d1 - zoff;
                        } else {
                            zz = Far;
                        }
                        set_z(zbuf[(yy + dy) * stride + dx + xx], zz);
                    }
                }
            }
        }
    }
}

void depthdequant(char *zbuf, const char *quantbuf, DepthFormat format, int depthps, int dx, int dy, int width,
                  int height, int stride)
{
#ifdef TIMING
    double start = Clock::time();
#endif

    if (stride < 0)
        stride = width;

    assert(dx + width <= stride);

    if (format == DepthFloat) {
        if (depthps <= 2) {
            depthdequant_t<DepthQuantize16, DepthQuantize16, float>(
                reinterpret_cast<float *>(zbuf), reinterpret_cast<const DepthQuantize16 *>(quantbuf),
                reinterpret_cast<const DepthQuantize16 *>(quantbuf), format, dx, dy, width, height, stride);
        } else {
            depthdequant_t<DepthQuantize24, DepthQuantize24, float>(
                reinterpret_cast<float *>(zbuf), reinterpret_cast<const DepthQuantize24 *>(quantbuf),
                reinterpret_cast<const DepthQuantize24 *>(quantbuf), format, dx, dy, width, height, stride);
        }
    } else {
        if (depthps <= 2) {
            depthdequant_t<DepthQuantize16, DepthQuantize16, uint16_t>(
                reinterpret_cast<uint16_t *>(zbuf), reinterpret_cast<const DepthQuantize16 *>(quantbuf),
                reinterpret_cast<const DepthQuantize16 *>(quantbuf), format, dx, dy, width, height, stride);
        } else {
            depthdequant_t<DepthQuantize24, DepthQuantize24, uint32_t>(
                reinterpret_cast<uint32_t *>(zbuf), reinterpret_cast<const DepthQuantize24 *>(quantbuf),
                reinterpret_cast<const DepthQuantize24 *>(quantbuf), format, dx, dy, width, height, stride);
        }
    }

#ifdef TIMING
    double t = Clock::time() - start;
    std::cerr << "depthdequant: " << t << " s, " << width * (height / 1000.) / 1000. << "MPix, "
              << 1. / t * width * height / 1000 / 1000 << " MPix/s" << std::endl;
#endif
}

void depthdequant_planar(char *zbuf, const char *quantbuf, DepthFormat format, int depthps, int dx, int dy, int width,
                         int height, int stride)
{
#ifdef TIMING
    double start = Clock::time();
#endif

    size_t edge = 1;
    if (depthps <= 2) {
        edge = DepthQuantize16::edge;
    } else {
        edge = DepthQuantize24::edge;
    }
    const size_t ntiles = ((width + edge - 1) / edge) * ((height + edge - 1) / edge);
    size_t mmsize16 = ntiles * sizeof(MinMaxDepth16);
    size_t mmsize24 = ntiles * sizeof(MinMaxDepth24);

    if (stride < 0)
        stride = width;

    assert(dx + width <= stride);

    vistle::buffer mmdepth;
    if (depthps <= 2) {
        mmdepth.resize(mmsize16);
        transform_unpredict<sizeof(MinMaxDepth16), true, false>((unsigned char *)mmdepth.data(),
                                                                (const unsigned char *)quantbuf, ntiles, 1, ntiles);
    } else {
        mmdepth.resize(mmsize24);
        transform_unpredict<sizeof(MinMaxDepth24), true, false>((unsigned char *)mmdepth.data(),
                                                                (const unsigned char *)quantbuf, ntiles, 1, ntiles);
    }

    if (format == DepthFloat) {
        if (depthps <= 2) {
            depthdequant_t<DepthQuantizeBits16, MinMaxDepth16, float>(
                reinterpret_cast<float *>(zbuf), reinterpret_cast<const DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<const MinMaxDepth16 *>(mmdepth.data()), format, dx, dy, width, height, stride);
        } else {
            depthdequant_t<DepthQuantizeBits24, MinMaxDepth24, float>(
                reinterpret_cast<float *>(zbuf), reinterpret_cast<const DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<const MinMaxDepth24 *>(mmdepth.data()), format, dx, dy, width, height, stride);
        }
    } else {
        if (depthps <= 2) {
            depthdequant_t<DepthQuantizeBits16, MinMaxDepth16, uint16_t>(
                reinterpret_cast<uint16_t *>(zbuf), reinterpret_cast<const DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<const MinMaxDepth16 *>(quantbuf), format, dx, dy, width, height, stride);
        } else {
            depthdequant_t<DepthQuantizeBits24, MinMaxDepth24, uint32_t>(
                reinterpret_cast<uint32_t *>(zbuf), reinterpret_cast<const DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<const MinMaxDepth24 *>(quantbuf), format, dx, dy, width, height, stride);
        }
    }

#ifdef TIMING
    double t = Clock::time() - start;
    std::cerr << "depthdequant_planar: " << t << " s, " << width * (height / 1000.) / 1000. << "MPix, "
              << 1. / t * width * height / 1000 / 1000 << " MPix/s" << std::endl;
#endif
}


template<int num>
static inline void sort(const uint32_t *depths, uint32_t *orders, uint32_t Far, uint32_t &maxidx)
{
    // insertion sort
    for (int i = 0; i < num; ++i) {
        orders[i] = i;
        for (int j = i; j > 0; --j) {
            if (depths[orders[j]] >= depths[orders[j - 1]])
                break;
            std::swap(orders[j], orders[j - 1]);
        }
    }

    // find index of largest element that is not Far
    maxidx = 0;
    for (int i = num - 1; i > 0; --i) {
        if (depths[orders[i]] != Far) {
            maxidx = i;
            break;
        }
    }
}

template<int size, uint32_t Far>
static inline void findgap(const uint32_t *depths, uint32_t midval, uint32_t &lowermid, uint32_t &uppermid)
{
    for (int i = 0; i < size; ++i) {
        if (depths[i] == Far)
            continue;
        if (depths[i] <= midval) {
            if (depths[i] > lowermid)
                lowermid = depths[i];
        } else {
            if (depths[i] < uppermid)
                uppermid = depths[i];
        }
    }
}

//! dxt-like compression for depth values
template<int bpp, class Quant, class MinMaxDepth, DepthFormat format>
static void depthquant_t(const uchar *inimg, Quant *quantbuf, MinMaxDepth *mmdepth, int x0, int y0, int w, int h,
                         int stride)
{
    const int edge = Quant::edge;
    const int size = edge * edge;
    const int scalebits = Quant::scale_bits;
    const int quantbits = Quant::bits_per_pixel;
    const uint32_t Far = 0x00ffffff;
    const uint32_t mask = (1U << quantbits) - 1;
    const uint32_t Valid = Quant::precision == 24 ? (Far & ~((1U << scalebits) - 1)) : 0x00ffff00;
    const uint32_t Next = Far - Valid + 1;

    assert(Valid <= Far);
    assert(Next > 0);
    assert((Next & Valid) > 0);
    assert(((Next - 1) & Valid) == 0);
#define NOCHECK
#if !defined(NOCHECK) || !defined(NDEBUG)
    const uint32_t qm2 = (1U << quantbits) / 2;
#endif

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int yy = y0; yy < y0 + h; yy += edge) {
        for (int xx = x0; xx < x0 + w; xx += edge) {
            uint32_t depths[size];
            bool haveFar = false;
            uint32_t mindepth = Far;
            uint32_t maxdepth = 0;

#define LOOP_BODY \
    const uint32_t d = get_depth<format, bpp>(inimg, x, y, stride, h); \
    depths[idx] = d; \
    if (d >= Far) { \
        haveFar = true; \
    } else { \
        if (d < mindepth) \
            mindepth = d; \
        if (d > maxdepth) \
            maxdepth = d; \
    }

            if (yy + edge <= y0 + h && xx + edge <= x0 + w) {
                const int ym = yy + edge, xm = xx + edge;
                int idx = 0;
                for (int y = yy; y < ym; ++y) {
                    for (int x = xx; x < xm; ++x) {
                        LOOP_BODY;
                        ++idx;
                    }
                }
            } else {
                for (int ty = 0; ty < edge; ++ty) {
                    int y = yy + ty;
                    if (y >= y0 + h)
                        y = y0 - h - 1;
                    for (int tx = 0; tx < edge; ++tx) {
                        int x = xx + tx;
                        if (x >= x0 + w)
                            x = x0 - w - 1;
                        const int idx = ty * edge + tx;
                        LOOP_BODY;
                    }
                }
            }

            if (mindepth == Far) {
                assert(haveFar == true);
                maxdepth = Far;
            }

            mindepth &= Valid;
            if (maxdepth != mindepth) {
                maxdepth &= Valid;
                if (maxdepth != (Far & Valid)) {
                    maxdepth += Next;
                }
                maxdepth += Next - 1;
                assert(haveFar || (mindepth & Valid) < (maxdepth & Valid));
            }

            assert(mindepth <= maxdepth);
            const float qscale = haveFar ? mask - 0.5f : mask + 0.5f;

            Quant &sq = quantbuf[((yy - y0) / edge) * ((w + edge - 1) / edge) + (xx - x0) / edge];
            MinMaxDepth &mmd = mmdepth[((yy - y0) / edge) * ((w + edge - 1) / edge) + (xx - x0) / edge];

            if (scalebits == 0) {
                const uint32_t range = maxdepth - mindepth;
                uint64_t bits = 0;
                if (range == 0) {
                    if (haveFar) {
                        bits = ~uint64_t(0);
                    } else {
                        bits = 0;
                    }
                } else {
                    if (haveFar) {
                        for (int idx = 0; idx < size; ++idx) {
                            const uint32_t depth = depths[idx];
                            uint32_t quant = 0;
                            if (depth == Far) {
                                quant = mask;
                            } else {
                                quant = ((depth - mindepth) * qscale) / range;
                                assert(quant < mask);
                            }

                            bits |= uint64_t(quant) << (idx * quantbits);
                        }
                    } else {
                        for (int idx = 0; idx < size; ++idx) {
                            const uint32_t depth = depths[idx];
                            uint32_t quant = ((depth - mindepth) * qscale) / range;
                            assert(quant <= mask);

                            bits |= uint64_t(quant) << (idx * quantbits);
                        }
                    }
                }
                dq_setbits(sq, bits);
            } else {
                const uint32_t scalemask = (1U << scalebits) - 1U;
                const uint32_t depthmask = ~scalemask;
                mindepth &= depthmask;
                maxdepth |= scalemask;
                const uint32_t range = maxdepth - mindepth;
                int lowerscale = 1, upperscale = 1;
                const uint32_t midval = (mindepth + maxdepth) >> 1;
                uint32_t lowermid = mindepth, uppermid = maxdepth;
                if (range > 1) {
                    findgap<size, Far>(depths, midval, lowermid, uppermid);
                    if (lowermid > mindepth)
                        lowerscale = range / (lowermid - mindepth) / 2;
                    if (maxdepth > uppermid)
                        upperscale = range / (maxdepth - uppermid) / 2;
                    if (lowerscale == 0)
                        lowerscale = 1;
                    if (upperscale == 0)
                        upperscale = 1;
                    if (lowerscale > 1 << scalebits)
                        lowerscale = 1 << scalebits;
                    if (upperscale > 1 << scalebits)
                        upperscale = 1 << scalebits;
                }

                uint64_t bits = 0;
                if (range == 0) {
                    if (haveFar) {
                        dq_setbits(sq, ~0UL);
                    } else {
                        dq_setbits(sq, 0UL);
                    }
                } else {
                    if (haveFar) {
                        for (int idx = 0; idx < size; ++idx) {
                            const uint32_t depth = depths[idx];
                            uint32_t quant = 0;

                            if (depth == Far) {
                                quant = mask;
                            } else if (depth <= midval) {
                                assert(depth <= lowermid);
                                quant = ((depth - mindepth) * (lowerscale * mask + 0.5f)) / range;
#ifndef NOCHECK
                                if (quant >= qm2)
                                    quant = qm2 - 1;
#endif
                                assert(quant < qm2);
                            } else {
                                assert(depth >= uppermid);
                                quant = ((maxdepth - depth) * (upperscale * (mask - 1.5f))) / range;
#ifndef NOCHECK
                                if (quant >= qm2 - 1)
                                    quant = qm2 - 2;
#endif
                                assert(quant < qm2 - 1);
                                quant = mask - 1 - quant;
                            }

                            bits |= uint64_t(quant) << (idx * quantbits);
                        }
                    } else {
                        for (int idx = 0; idx < size; ++idx) {
                            const uint32_t depth = depths[idx];
                            uint32_t quant = 0;

                            if (depth <= midval) {
                                assert(depth <= lowermid);
                                quant = ((depth - mindepth) * (lowerscale * mask + 0.5f)) / range;
#ifndef NOCHECK
                                if (quant >= qm2)
                                    quant = qm2 - 1;
#endif
                                assert(quant < qm2);
                            } else {
                                assert(depth >= uppermid);
                                assert(depth < Far);
                                quant = ((maxdepth - depth) * (upperscale * mask + 0.5f)) / range;
#ifndef NOCHECK
                                if (quant >= qm2)
                                    quant = qm2 - 1;
#endif
                                assert(quant < qm2);
                                quant = mask - quant;
                            }

                            bits |= uint64_t(quant) << (idx * quantbits);
                        }
                    }
                    dq_setbits(sq, bits);
                }

                --upperscale;
                --lowerscale;

                mindepth &= depthmask;
                mindepth |= lowerscale;
                maxdepth &= depthmask;
                maxdepth |= upperscale;
            }

            if (haveFar) {
                setdepth(mmd, 0, maxdepth);
                setdepth(mmd, 1, mindepth);
            } else {
                setdepth(mmd, 0, mindepth);
                setdepth(mmd, 1, maxdepth);
            }
        }
    }
}

void depthquant(char *quantbufS, const char *zbufS, DepthFormat format, int depthps, int x, int y, int width,
                int height, int stride)
{
#ifdef TIMING
    double start = Clock::time();
#endif

    if (stride < 0)
        stride = width;

    assert(x + width <= stride);

    uchar *quantbuf = (uchar *)quantbufS;
    const uchar *zbuf = (const uchar *)zbufS;

    switch (format) {
    case DepthFloat:
        if (depthps <= 2) {
            depthquant_t<4, DepthQuantize16, DepthQuantize16, DepthFloat>(
                zbuf, reinterpret_cast<DepthQuantize16 *>(quantbuf), reinterpret_cast<DepthQuantize16 *>(quantbuf), x,
                y, width, height, stride);
        } else {
            depthquant_t<4, DepthQuantize24, DepthQuantize24, DepthFloat>(
                zbuf, reinterpret_cast<DepthQuantize24 *>(quantbuf), reinterpret_cast<DepthQuantize24 *>(quantbuf), x,
                y, width, height, stride);
        }
        break;
    case DepthRGBA:
        if (depthps <= 2) {
            depthquant_t<4, DepthQuantize16, DepthQuantize16, DepthRGBA>(
                zbuf, reinterpret_cast<DepthQuantize16 *>(quantbuf), reinterpret_cast<DepthQuantize16 *>(quantbuf), x,
                y, width, height, stride);
        } else {
            depthquant_t<4, DepthQuantize24, DepthQuantize24, DepthRGBA>(
                zbuf, reinterpret_cast<DepthQuantize24 *>(quantbuf), reinterpret_cast<DepthQuantize24 *>(quantbuf), x,
                y, width, height, stride);
        }
        break;
    case DepthInteger:
        if (depthps == 1) {
            depthquant_t<1, DepthQuantize16, DepthQuantize16, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantize16 *>(quantbuf), reinterpret_cast<DepthQuantize16 *>(quantbuf), x,
                y, width, height, stride);
        } else if (depthps == 2) {
            depthquant_t<2, DepthQuantize16, DepthQuantize16, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantize16 *>(quantbuf), reinterpret_cast<DepthQuantize16 *>(quantbuf), x,
                y, width, height, stride);
        } else if (depthps == 3) {
            depthquant_t<3, DepthQuantize24, DepthQuantize24, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantize24 *>(quantbuf), reinterpret_cast<DepthQuantize24 *>(quantbuf), x,
                y, width, height, stride);
        } else if (depthps == 4) {
            depthquant_t<4, DepthQuantize24, DepthQuantize24, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantize24 *>(quantbuf), reinterpret_cast<DepthQuantize24 *>(quantbuf), x,
                y, width, height, stride);
        }
        break;
    default:
        assert("unsupported combination of format and bytes/pixel" == NULL);
    }

#ifdef TIMING
    double t = Clock::time() - start;
    std::cerr << "depthquant: " << t << " s, " << width * (height / 1000.) / 1000. << "MPix, "
              << 1. / t * width * height / 1000 / 1000 << " MPix/s" << std::endl;
#endif
}

void depthquant_planar(char *quantbufS, const char *zbufS, DepthFormat format, int depthps, int x, int y, int width,
                       int height, int stride)
{
#ifdef TIMING
    double start = Clock::time();
#endif

    if (stride < 0)
        stride = width;

    assert(x + width <= stride);

    uchar *quantbuf = (uchar *)quantbufS;
    const uchar *zbuf = (const uchar *)zbufS;

    size_t edge = 1;
    if (depthps <= 2) {
        edge = DepthQuantize16::edge;
    } else {
        edge = DepthQuantize24::edge;
    }
    const size_t ntiles = ((width + edge - 1) / edge) * ((height + edge - 1) / edge);
    size_t mmsize16 = ntiles * sizeof(MinMaxDepth16);
    size_t mmsize24 = ntiles * sizeof(MinMaxDepth24);

    vistle::buffer mmdepth;
    if (depthps <= 2) {
        mmdepth.resize(mmsize16);
    } else {
        mmdepth.resize(mmsize24);
    }

    switch (format) {
    case DepthFloat:
        if (depthps <= 2) {
            depthquant_t<4, DepthQuantizeBits16, MinMaxDepth16, DepthFloat>(
                zbuf, reinterpret_cast<DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<MinMaxDepth16 *>(mmdepth.data()), x, y, width, height, stride);
        } else {
            depthquant_t<4, DepthQuantizeBits24, MinMaxDepth24, DepthFloat>(
                zbuf, reinterpret_cast<DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<MinMaxDepth24 *>(mmdepth.data()), x, y, width, height, stride);
        }
        break;
    case DepthRGBA:
        if (depthps <= 2) {
            depthquant_t<4, DepthQuantizeBits16, MinMaxDepth16, DepthRGBA>(
                zbuf, reinterpret_cast<DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<MinMaxDepth16 *>(mmdepth.data()), x, y, width, height, stride);
        } else {
            depthquant_t<4, DepthQuantizeBits24, MinMaxDepth24, DepthRGBA>(
                zbuf, reinterpret_cast<DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<MinMaxDepth24 *>(mmdepth.data()), x, y, width, height, stride);
        }
        break;
    case DepthInteger:
        if (depthps == 1) {
            depthquant_t<1, DepthQuantizeBits16, MinMaxDepth16, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<MinMaxDepth16 *>(mmdepth.data()), x, y, width, height, stride);
        } else if (depthps == 2) {
            depthquant_t<2, DepthQuantizeBits16, MinMaxDepth16, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantizeBits16 *>(quantbuf + mmsize16),
                reinterpret_cast<MinMaxDepth16 *>(mmdepth.data()), x, y, width, height, stride);
        } else if (depthps == 3) {
            depthquant_t<3, DepthQuantizeBits24, MinMaxDepth24, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<MinMaxDepth24 *>(mmdepth.data()), x, y, width, height, stride);
        } else if (depthps == 4) {
            depthquant_t<4, DepthQuantizeBits24, MinMaxDepth24, DepthInteger>(
                zbuf, reinterpret_cast<DepthQuantizeBits24 *>(quantbuf + mmsize24),
                reinterpret_cast<MinMaxDepth24 *>(mmdepth.data()), x, y, width, height, stride);
        }
        break;
    default:
        assert("unsupported combination of format and bytes/pixel" == NULL);
    }

    if (depthps <= 2) {
        transform_predict<sizeof(MinMaxDepth16), true, false>(quantbuf, (const unsigned char *)mmdepth.data(), ntiles,
                                                              1, ntiles);
    } else {
        transform_predict<sizeof(MinMaxDepth24), true, false>(quantbuf, (const unsigned char *)mmdepth.data(), ntiles,
                                                              1, ntiles);
    }


#ifdef TIMING
    double t = Clock::time() - start;
    std::cerr << "depthquant_planar: " << t << " s, " << width * (height / 1000.) / 1000. << "MPix, "
              << 1. / t * width * height / 1000 / 1000 << " MPix/s" << std::endl;
#endif
}

} // namespace vistle
