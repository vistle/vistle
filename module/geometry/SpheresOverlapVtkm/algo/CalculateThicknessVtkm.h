#ifndef VISTLE_SPHERESOVERLAPVTKM_ALGO_CALCULATETHICKNESSVTKM_H
#define VISTLE_SPHERESOVERLAPVTKM_ALGO_CALCULATETHICKNESSVTKM_H

#include <vistle/core/scalar.h>
#include <vistle/util/enum.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ThicknessDeterminer, (Overlap)(OverlapRatio)(Distance));

VISKORES_EXEC
vistle::Scalar CalculateThicknessVtkm(ThicknessDeterminer determiner, vistle::Scalar distance, vistle::Scalar radius1,
                                      vistle::Scalar radius2);

#endif
