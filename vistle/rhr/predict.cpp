#include "predict.h"

#include <util/math.h>
#include <cstdint>
#include <cassert>

namespace vistle {

namespace {
const uint32_t Max = 0xffffffU;
}


void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride) {

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {

        const float *in = input + y*stride;
        unsigned char *out = output + y*width*3;

        uint8_t prev[3] = { 0, 0, 0 };
        for (unsigned x = 0; x < width; ++x) {
            uint32_t I = *in * Max;
            if (I > Max)
                I = Max;
            ++in;

            for (unsigned i=0; i<3; ++i) {
                uint8_t a = I & 0xff;
                I >>= 8;
                uint8_t d = a - prev[i];
                prev[i] = a;
                out[i] = d;
            }

            out+= 3;
        }
    }
}

void transform_unpredict(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride) {

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {

        float *out = output + y*stride;
        const unsigned char *in = input + y*width*3;

        uint8_t prev[3] = { 0, 0, 0 };
        for (unsigned x = 0; x < width; ++x) {

            for (unsigned i = 0; i < 3; ++i) {

                uint8_t d = in[i];
                uint8_t a = prev[i] + d;
                prev[i] = a;
            }
            uint32_t I = prev[0] | (uint32_t(prev[1])<<8) | (uint32_t(prev[2])<<16);

            in += 3;
            *out = (float)I / Max;
            ++out;
        }
    }
}

void transform_predict_planar(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride) {

    const size_t plane_size = width*height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {

        const float *in = input + y*stride;
        unsigned char *out[3] = {
            output + y*width,
            output + plane_size + y*width,
            output + 2*plane_size + y*width
        };

        uint8_t prev[3] = { 0, 0, 0 };
        for (unsigned x = 0; x < width; ++x) {
            uint32_t I = *in * Max;
            if (I > Max)
                I = Max;
            ++in;

            for (unsigned i=0; i<3; ++i) {
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

void transform_unpredict_planar(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride) {

    const size_t plane_size = width*height;

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (unsigned y = 0; y < height; ++y) {

        float *out = output + y*stride;
        const unsigned char *in[3] = {
            input + y*width,
            input + plane_size + y*width,
            input + 2*plane_size + y*width
        };

        uint8_t prev[3] = { 0, 0, 0 };
        for (unsigned x = 0; x < width; ++x) {

            for (unsigned i = 0; i < 3; ++i) {

                uint8_t d = *in[i];
                ++in[i];
                uint8_t a = prev[i] + d;
                prev[i] = a;
            }
            uint32_t I = prev[0] | (uint32_t(prev[1])<<8) | (uint32_t(prev[2])<<16);

            *out = (float)I / Max;
            ++out;
        }
    }
}

}
