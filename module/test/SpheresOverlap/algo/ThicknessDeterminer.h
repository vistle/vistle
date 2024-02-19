#ifndef THICKNESS_DETERMINER_H
#define THICKNESS_DETERMINER_H

#include <vistle/core/scalar.h>
#include <vistle/util/enum.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ThicknessDeterminer, (Overlap)(OverlapRatio)(Distance));

vistle::Scalar CalculateThickness(ThicknessDeterminer determiner, vistle::Scalar distance, vistle::Scalar radius1,
                                  vistle::Scalar radius2);

#endif // THICKNESS_DETERMINER_H