#ifndef VISTLE_RHR_PREDICT_H
#define VISTLE_RHR_PREDICT_H

#include <cstdlib>

namespace vistle {

void transform_predict(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride);
void transform_unpredict(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
void transform_predict_planar(unsigned char *output, const float *input, unsigned width, unsigned height, unsigned stride);
void transform_unpredict_planar(float *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);

template<int planes, bool planar=true, bool color=false>
void transform_predict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
template<int planes, bool planar=true, bool color=false>
void transform_unpredict(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);

#if 0
extern template
void transform_predict<1,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_predict<2,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_predict<3,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_predict<4,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_predict<5,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_predict<6,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);

extern template
void transform_unpredict<1,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_unpredict<2,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_unpredict<3,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_unpredict<4,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_unpredict<5,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
extern template
void transform_unpredict<6,true,false>(unsigned char *output, const unsigned char *input, unsigned width, unsigned height, unsigned stride);
#endif

}
#endif
