#ifndef VISTLE_RHR_PREDICT_H
#define VISTLE_RHR_PREDICT_H

#include <cstdlib>

namespace vistle {

void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride);
void transform_unpredict(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
void transform_predict_planar(unsigned char *output, const float *input, unsigned width, unsigned height,
                              unsigned stride);
void transform_unpredict_planar(float *output, const unsigned char *input, unsigned width, unsigned height,
                                unsigned stride);

template<int planes, bool planar = true, bool rgba = false>
void transform_predict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height,
                       unsigned stride);
template<int planes, bool planar = true, bool rgba = false>
void transform_unpredict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height,
                         unsigned stride);

} // namespace vistle
#endif
