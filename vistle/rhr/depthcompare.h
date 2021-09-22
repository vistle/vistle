#ifndef VISTLE_RHR_DEPTHCOMPARE_H
#define VISTLE_RHR_DEPTHCOMPARE_H

#include "depthquant.h"

namespace vistle {

//! compute (and optionally print) PSNR for compressed depth image check compared to reference image ref
double depthcompare(const char *ref, const char *check, DepthFormat format, int depthps, int x, int y, int w, int h,
                    int stride, bool print = true);

} // namespace vistle
#endif
