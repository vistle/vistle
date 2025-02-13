#ifndef VISTLE_CORE_SCALAR_H
#define VISTLE_CORE_SCALAR_H

#include <cinttypes>

namespace vistle {

// used for mapped data
typedef uint8_t Byte;
typedef double Scalar64;
typedef float Scalar32;

#ifdef VISTLE_SCALAR_DOUBLE
typedef Scalar64 Scalar; // also used for coordinates
#else
typedef Scalar32 Scalar; // also used for coordinates
#endif

// used for parameters
typedef double Float;
typedef int64_t Integer;

} // namespace vistle

#endif
