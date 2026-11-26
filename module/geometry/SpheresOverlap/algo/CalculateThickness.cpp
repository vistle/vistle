#include <cmath>
#include <stdexcept>

#include "CalculateThickness.h"

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
        throw std::invalid_argument("unknown thickness determiner");
    }
}
