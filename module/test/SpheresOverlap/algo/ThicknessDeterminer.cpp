#include <cmath>

#include <vtkm/Types.h>

#include "ThicknessDeterminer.h"

VTKM_EXEC_CONT
vistle::Scalar CalculateThickness(ThicknessDeterminer determiner, vistle::Scalar distance, vistle::Scalar radius1,
                                  vistle::Scalar radius2)
{
    switch (determiner) {
    case Overlap:
        return std::abs(radius1 + radius2 - distance);
    case OverlapRatio:
        return std::abs(radius1 + radius2 - distance) / distance;
    case Distance:
        return distance;
    default:
        // unknown thickness determiner
        // can't throw exception because device code doesn't support it
        VTKM_ASSERT(false);
    }
}