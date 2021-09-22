#include "depthcompare.h"

#include <cstdio>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>
#include <sstream>

#include <vistle/util/netpbmimage.h>

//#define TIMING

#ifdef TIMING
#include "stopwatch.h"
#endif

#include <cassert>

//#define ERRORPIC


namespace vistle {

template<typename T>
static int clz(T xx)
{
    assert(sizeof(T) <= 8);
    uint64_t x = xx;

    if (xx == 0)
        return 8 * sizeof(T);

    int n = 0;
    if ((x & 0xffffffff00000000ULL) == 0) {
        x <<= 32;
        if (sizeof(T) >= 8)
            n += 32;
    }
    if ((x & 0xffff000000000000ULL) == 0) {
        x <<= 16;
        if (sizeof(T) >= 4)
            n += 16;
    }
    if ((x & 0xff00000000000000ULL) == 0) {
        x <<= 8;
        if (sizeof(T) >= 2)
            n += 8;
    }
    if ((x & 0xf000000000000000ULL) == 0) {
        x <<= 4;
        n += 4;
    }
    if ((x & 0xc000000000000000ULL) == 0) {
        x <<= 2;
        n += 2;
    }
    if ((x & 0x8000000000000000ULL) == 0) {
        x <<= 1;
        n += 1;
    }
    return n;
}


template<typename T>
double depthcompare_t(const T *ref, const T *check, int xx, int yy, int w, int h, int stride, int precision,
                      int bits_per_pixel, bool print)
{
    size_t numlow = 0, numhigh = 0;
    unsigned maxx = 0, maxy = 0;
    size_t numblack = 0, numblackwrong = 0;
    size_t totalvalidbits = 0;
    int minvalidbits = 8 * sizeof(T), maxvalidbits = 0;
    double totalerror = 0.;
    double squarederror = 0.;
    T Max = std::numeric_limits<T>::max();
    if (sizeof(T) == 4)
        Max >>= 8;
    T maxerr = 0;
    T refminval = Max, minval = Max;
    T refmaxval = 0, maxval = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = y * w + x;
            T e = 0;

            T r = ref[(yy + y) * stride + xx + x];
            T c = check[idx];
            if (sizeof(T) == 4) {
                r >>= 8;
                c >>= 8;
            }
            if (r == Max) {
                ++numblack;
                if (c != r) {
                    ++numblackwrong;
                    if (print)
                        fprintf(stderr, "!B: %u %u (%lu)\n", x, y, (unsigned long)c);
                }
            } else {
                int validbits = clz<T>(r ^ c);
                if (validbits < minvalidbits)
                    minvalidbits = validbits;
                if (validbits > maxvalidbits)
                    maxvalidbits = validbits;
                totalvalidbits += validbits;

                if (r < refminval)
                    refminval = r;
                if (r > refmaxval)
                    refmaxval = r;
                if (c < minval)
                    minval = c;
                if (c > maxval)
                    maxval = c;
            }
            if (r > c) {
                ++numlow;
                e = r - c;
            } else if (r < c) {
                ++numhigh;
                e = c - r;
            }
            if (e > maxerr) {
                maxx = x;
                maxy = y;
                maxerr = e;
            }
            double err = e;
            totalerror += err;
            squarederror += err * err;
        }
    }

    size_t nonblack = w * h - numblack;
    double MSE = squarederror / w / h;
    double MSE_nonblack = squarederror / nonblack;
    double PSNR = -1., PSNR_nonblack = -1.;
    if (MSE > 0.) {
        PSNR = 10. * (2. * log10(Max) - log10(MSE));
        PSNR_nonblack = 10. * (2. * log10(Max) - log10(MSE_nonblack));
    }

    if (print) {
        fprintf(stderr, "ERROR: #high=%ld, #low=%ld, max=%lu (%u %u), total=%f, mean non-black=%f (%f %%)\n",
                (long)numhigh, (long)numlow, (unsigned long)maxerr, maxx, maxy, totalerror, totalerror / nonblack,
                totalerror / nonblack * 100. / (double)Max);
#if 0
      fprintf(stderr, "BITS: totalvalid=%lu, min=%d, max=%d, mean=%f\n",
            totalvalidbits, minvalidbits, maxvalidbits, (double)totalvalidbits/(double)nonblack);
#endif

        fprintf(stderr, "PSNR (2x%d+16x%d): %f dB (non-black: %f)\n", precision, bits_per_pixel, PSNR, PSNR_nonblack);

#if 0
      fprintf(stderr, "RANGE: ref %lu - %lu, act %lu - %lu\n",
            (unsigned long)refminval, (unsigned long)refmaxval,
            (unsigned long)minval, (unsigned long)maxval);
#endif
    }

    return PSNR;
}

template<>
double depthcompare_t(const float *ref, const float *check, int xx, int yy, int w, int h, int stride, int precision,
                      int bits_per_pixel, bool print)
{
#ifdef ERRORPIC
    static int count = 0;
    std::stringstream filename;
    filename << "dq_error_" << count << ".ppm";
    ++count;
    vistle::NetpbmImage img(filename.str(), w, h, vistle::NetpbmImage::PGM);
#endif

    typedef float T;
    size_t numlow = 0, numhigh = 0;
    unsigned maxx = 0, maxy = 0;
    size_t numblack = 0, numblackwrong = 0;
    double totalerror = 0.;
    double squarederror = 0.;
    T Max = 1.f;
    T maxerr = 0;
    T refminval = Max, minval = Max;
    T refmaxval = 0, maxval = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = y * w + x;
            T e = 0;

            T r = ref[(yy + y) * stride + xx + x];
            T c = check[idx];
#ifdef ERRORPIC
            img.append(fabs(r - c));
#endif
            if (r == Max) {
                ++numblack;
                if (c != r) {
                    ++numblackwrong;
                    if (print)
                        fprintf(stderr, "!B f: %u %u (%lu)\n", x, y, (unsigned long)c);
                }
            } else {
                if (r < refminval)
                    refminval = r;
                if (r > refmaxval)
                    refmaxval = r;
                if (c < minval)
                    minval = c;
                if (c > maxval)
                    maxval = c;
            }
            if (r > c) {
                ++numlow;
                e = r - c;
            } else if (r < c) {
                ++numhigh;
                e = c - r;
            }
            if (e > maxerr) {
                maxx = x;
                maxy = y;
                maxerr = e;
            }
            double err = e;
            totalerror += err;
            squarederror += err * err;
        }
    }

    size_t nonblack = w * h - numblack;
    double MSE = squarederror / w / h;
    double MSE_nonblack = squarederror / nonblack;
    double PSNR = -1., PSNR_nonblack = -1.;
    if (MSE > 0.) {
        PSNR = 10. * (2. * log10(Max) - log10(MSE));
        PSNR_nonblack = 10. * (2. * log10(Max) - log10(MSE_nonblack));
    }

    if (print) {
        fprintf(stderr, "ERROR: #high=%ld, #low=%ld, max=%lu (%u %u), total=%f, mean non-black=%f (%f %%)\n",
                (long)numhigh, (long)numlow, (unsigned long)maxerr, maxx, maxy, totalerror, totalerror / nonblack,
                totalerror / nonblack * 100. / (double)Max);

        fprintf(stderr, "PSNR float (2x%d+16x%d): %f dB (non-black: %f)\n", precision, bits_per_pixel, PSNR,
                PSNR_nonblack);

#if 0
      fprintf(stderr, "RANGE: ref %lu - %lu, act %lu - %lu\n",
            (unsigned long)refminval, (unsigned long)refmaxval,
            (unsigned long)minval, (unsigned long)maxval);
#endif
    }

#ifdef ERRORPIC
    img.close();
    std::cerr << "Written: " << img << std::endl;
    vistle::NetpbmImage img2(filename.str());
    std::cerr << "Read:    " << img2 << std::endl;
#endif

    return PSNR;
}

double depthcompare(const char *ref, const char *check, DepthFormat format, int depthps, int x, int y, int w, int h,
                    int stride, bool print)
{
    int precision = 0, bits_per_pixel = 0;
    if (depthps == 2) {
        precision = DepthQuantize16::precision;
        bits_per_pixel = DepthQuantize16::bits_per_pixel;
    } else if (depthps == 4 || depthps == 3) {
        precision = DepthQuantize24::precision;
        bits_per_pixel = DepthQuantize24::bits_per_pixel;
    }

    if (format == DepthFloat) {
        return depthcompare_t<float>((const float *)ref, (const float *)check, x, y, w, h, stride, precision,
                                     bits_per_pixel, print);
    } else {
        if (depthps == 2) {
            return depthcompare_t<uint16_t>((const uint16_t *)ref, (const uint16_t *)check, x, y, w, h, stride,
                                            precision, bits_per_pixel, print);
        } else if (depthps == 4) {
            return depthcompare_t<uint32_t>((const uint32_t *)ref, (const uint32_t *)check, x, y, w, h, stride,
                                            precision, bits_per_pixel, print);
        }
    }

    return -1.;
}

} // namespace vistle
