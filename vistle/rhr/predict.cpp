#include "predict.h"

#include <util/math.h>
#include <cstdint>
#include <cassert>

namespace vistle {

void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride) {

    const uint32_t Max = 0xffffffU;

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

    const uint32_t F = 1<<24;

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
            *out = (float)I / F;
            ++out;
        }
    }
}

}
