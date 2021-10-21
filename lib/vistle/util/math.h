#ifndef VISTLE_MATH_H
#define VISTLE_MATH_H

#include <algorithm>
#include <cmath>

namespace vistle {

template<typename S>
S clamp(S v, S vmin, S vmax)
{
    return std::min(std::max(v, vmin), vmax);
}

template<typename S, typename Float>
inline S lerp(S a, S b, Float t)
{
    return a + t * (b - a);
}

template<typename S>
inline S difference_of_products(S a, S b, S c, S d)
{
    S cd = c * d;
    S err = std::fma(-c, d, cd);
    S dop = std::fma(a, b, -cd);
    return dop + err;
}

} // namespace vistle
#endif
