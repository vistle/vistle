#ifndef VISTLE_SPHERESOVERLAP_ALGO_THICKNESSDETERMINER_H
#define VISTLE_SPHERESOVERLAP_ALGO_THICKNESSDETERMINER_H

#include <vistle/core/scalar.h>
#include <vistle/util/enum.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ThicknessDeterminer, (Overlap)(OverlapRatio)(Distance));

VISKORES_EXEC_CONT
vistle::Scalar CalculateThickness(ThicknessDeterminer determiner, vistle::Scalar distance, vistle::Scalar radius1,
                                  vistle::Scalar radius2);

#endif
