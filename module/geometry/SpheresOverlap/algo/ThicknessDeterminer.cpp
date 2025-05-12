#include <viskores/Math.h>
#include <viskores/Types.h>

#include "ThicknessDeterminer.h"

VISKORES_EXEC_CONT
vistle::Scalar CalculateThickness(ThicknessDeterminer determiner, vistle::Scalar distance, vistle::Scalar radius1,
                                  vistle::Scalar radius2)
{
    switch (determiner) {
    case Overlap:
        return viskores::Abs(radius1 + radius2 - distance);
    case OverlapRatio:
        return viskores::Abs(radius1 + radius2 - distance) / distance;
    case Distance:
        return distance;
    default:
        // unknown thickness determiner
        // can't throw exception because device code doesn't support it
        VISKORES_ASSERT(false);
        return 0;
    }
}
