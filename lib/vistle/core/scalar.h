#ifndef VISTLE_SCALAR_H
#define VISTLE_SCALAR_H

#include <cinttypes>

namespace vistle {

// used for mapped data
typedef uint8_t Byte;

#ifdef VISTLE_SCALAR_DOUBLE
typedef double Scalar; // also used for coordinates
#else
typedef float Scalar; // also used for coordinates
#endif

// used for parameters
typedef double Float;
typedef int64_t Integer;

} // namespace vistle

#endif
