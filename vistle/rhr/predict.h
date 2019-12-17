#ifndef VISTLE_RHR_PREDICT_H
#define VISTLE_RHR_PREDICT_H

#include <cstdlib>

namespace vistle {

void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride);
void transform_unpredict(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);

}
#endif
