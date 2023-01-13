#include "predict.h"

#include <vistle/util/math.h>
#include <cstdint>
#include <cassert>

namespace vistle {

namespace {
const uint32_t Max = 0xffffffU;
}


void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        const float *in = input + y * stride;
        unsigned char *out = output + y * width * 3;

        uint8_t prev[3] = {0, 0, 0};
        for (unsigned x = 0; x < width; ++x) {
            uint32_t I = *in * Max;
            if (I > Max)
                I = Max;
            ++in;

            for (unsigned i = 0; i < 3; ++i) {
                uint8_t a = I & 0xff;
                I >>= 8;
                uint8_t d = a - prev[i];
                prev[i] = a;
                out[i] = d;
            }

            out += 3;
        }
    }
}

void transform_unpredict(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride)
{
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        float *out = output + y * stride;
        const unsigned char *in = input + y * width * 3;

        uint8_t prev[3] = {0, 0, 0};
        for (unsigned x = 0; x < width; ++x) {
            for (unsigned i = 0; i < 3; ++i) {
                uint8_t d = in[i];
                uint8_t a = prev[i] + d;
                prev[i] = a;
            }
            uint32_t I = prev[0] | (uint32_t(prev[1]) << 8) | (uint32_t(prev[2]) << 16);

            in += 3;
            *out = (float)I / Max;
            ++out;
        }
    }
}

void transform_predict_planar(unsigned char *output, const float *input, unsigned width, unsigned height,
                              unsigned stride)
{
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        const float *in = input + y * stride;
        unsigned char *out[3] = {output + y * width, output + plane_size + y * width,
                                 output + 2 * plane_size + y * width};

        uint8_t prev[3] = {0, 0, 0};
        for (unsigned x = 0; x < width; ++x) {
            uint32_t I = *in * Max;
            if (I > Max)
                I = Max;
            ++in;

            for (unsigned i = 0; i < 3; ++i) {
                uint8_t a = I & 0xff;
                I >>= 8;
                uint8_t d = a - prev[i];
                prev[i] = a;

                *out[i] = d;
                ++out[i];
            }
        }
    }
}

void transform_unpredict_planar(float *output, const unsigned char *input, unsigned width, unsigned height,
                                unsigned stride)
{
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        float *out = output + y * stride;
        const unsigned char *in[3] = {input + y * width, input + plane_size + y * width,
                                      input + 2 * plane_size + y * width};

        uint8_t prev[3] = {0, 0, 0};
        for (unsigned x = 0; x < width; ++x) {
            for (unsigned i = 0; i < 3; ++i) {
                uint8_t d = *in[i];
                ++in[i];
                uint8_t a = prev[i] + d;
                prev[i] = a;
            }
            uint32_t I = prev[0] | (uint32_t(prev[1]) << 8) | (uint32_t(prev[2]) << 16);

            *out = (float)I / Max;
            ++out;
        }
    }
}

template<int planes, bool planar, bool rgba>
void transform_predict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height,
                       unsigned stride)
{
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        const unsigned char *in = input + y * stride * planes;
        unsigned char *out[planes];
        for (int i = 0; i < planes; ++i) {
            out[i] = output + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            for (unsigned p = 0; p < planes; ++p) {
                uint8_t d = *in;
                *out[p] = (d - prev[p]) & 0xff;
                prev[p] = d;
                ++in;
                ++out[p];
            }
        }
    }
}

template<int planes, bool planar, bool rgba>
void transform_unpredict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height,
                         unsigned stride)
{
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        unsigned char *out = output + y * stride * planes;
        const unsigned char *in[planes];
        for (int i = 0; i < planes; ++i) {
            in[i] = input + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            for (unsigned p = 0; p < planes; ++p) {
                uint8_t a = prev[p] + *in[p];
                *out = a & 0xff;
                prev[p] = a;
                ++in[p];
                ++out;
            }
        }
    }
}


template void transform_predict<1, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);
template void transform_predict<2, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);
template void transform_predict<3, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);
template void transform_predict<4, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);
template void transform_predict<5, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);
template void transform_predict<6, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                unsigned height, unsigned stride);

template void transform_unpredict<1, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);
template void transform_unpredict<2, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);
template void transform_unpredict<3, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);
template void transform_unpredict<4, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);
template void transform_unpredict<5, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);
template void transform_unpredict<6, true, false>(unsigned char *output, const unsigned char *input, unsigned width,
                                                  unsigned height, unsigned stride);

// for color data

static void rgb2yuv(uint8_t r, uint8_t g, uint8_t b, uint8_t &y, uint8_t &u, uint8_t &v)
{
    y = b;
    u = g - b;
    v = g - r;
}

static void yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
{
    b = y;
    g = u + b;
    r = g - v;
}

template<>
void transform_predict<3, true, true>(unsigned char *output, const unsigned char *input, unsigned width,
                                      unsigned height, unsigned stride)
{
    const int planes = 3;
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        const unsigned char *in = input + y * stride * 4;
        unsigned char *out[planes];
        for (int i = 0; i < planes; ++i) {
            out[i] = output + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            uint8_t r = *in++;
            uint8_t g = *in++;
            uint8_t b = *in++;
            uint8_t a = *in++;
            (void)a;

            uint8_t y, u, v;
            rgb2yuv(r, g, b, y, u, v);

            *out[0]++ = y - prev[0];
            *out[1]++ = u - prev[1];
            *out[2]++ = v - prev[2];

            prev[0] = y;
            prev[1] = u;
            prev[2] = v;
        }
    }
}

template<>
void transform_unpredict<3, true, true>(unsigned char *output, const unsigned char *input, unsigned width,
                                        unsigned height, unsigned stride)
{
    const int planes = 3;
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        unsigned char *out = output + y * stride * 4;
        const unsigned char *in[planes];
        for (int i = 0; i < planes; ++i) {
            in[i] = input + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            uint8_t y = prev[0] + *in[0]++;
            prev[0] = y;
            uint8_t u = prev[1] + *in[1]++;
            prev[1] = u;
            uint8_t v = prev[2] + *in[2]++;
            prev[2] = v;

            uint8_t r, g, b;
            yuv2rgb(y, u, v, r, g, b);

            *out++ = r;
            *out++ = g;
            *out++ = b;
            *out++ = 0xff;
        }
    }
}

template<>
void transform_predict<4, true, true>(unsigned char *output, const unsigned char *input, unsigned width,
                                      unsigned height, unsigned stride)
{
    const int planes = 4;
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        const unsigned char *in = input + y * stride * planes;
        unsigned char *out[planes];
        for (int i = 0; i < planes; ++i) {
            out[i] = output + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            uint8_t r = *in++;
            uint8_t g = *in++;
            uint8_t b = *in++;
            uint8_t a = *in++;

            uint8_t y, u, v;
            rgb2yuv(r, g, b, y, u, v);

            *out[0]++ = y - prev[0];
            *out[1]++ = u - prev[1];
            *out[2]++ = v - prev[2];
            *out[3]++ = a - prev[3];

            prev[0] = y;
            prev[1] = u;
            prev[2] = v;
            prev[3] = a;
        }
    }
}

template<>
void transform_unpredict<4, true, true>(unsigned char *output, const unsigned char *input, unsigned width,
                                        unsigned height, unsigned stride)
{
    const int planes = 4;
    const size_t plane_size = width * height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {
        unsigned char *out = output + y * stride * planes;
        const unsigned char *in[planes];
        for (int i = 0; i < planes; ++i) {
            in[i] = input + i * plane_size + y * width;
        }

        uint8_t prev[planes] = {};
        for (unsigned x = 0; x < width; ++x) {
            uint8_t y = prev[0] + *in[0]++;
            prev[0] = y;
            uint8_t u = prev[1] + *in[1]++;
            prev[1] = u;
            uint8_t v = prev[2] + *in[2]++;
            prev[2] = v;
            uint8_t a = prev[3] + *in[3]++;
            prev[3] = a;

            uint8_t r, g, b;
            yuv2rgb(y, u, v, r, g, b);

            *out++ = r;
            *out++ = g;
            *out++ = b;
            *out++ = a;
        }
    }
}

} // namespace vistle
