#ifndef MAIAVISPOINT_H_
#define MAIAVISPOINT_H_

#include <array>
#include "INCLUDE/maiatypes.h"

namespace maiapv{
// Structure used to represent points
template <MInt nDim> using Point = std::array<MFloat, nDim>;
} // namespace maiapv

#endif

