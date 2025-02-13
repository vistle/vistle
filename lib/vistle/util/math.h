#ifndef VISTLE_UTIL_MATH_H
#define VISTLE_UTIL_MATH_H

#include <algorithm>
#include <cmath>

namespace vistle {

//! return v clamped to interval [vmin,vmax]
template<typename S>
S clamp(S v, S vmin, S vmax)
{
    return std::min(std::max(v, vmin), vmax);
}

//! linearly interpolate between a and b with weight t
template<typename S, typename Float>
inline S lerp(S a, S b, Float t)
{
    return a + t * (b - a);
}

//! return weight for linear interpolation at between from low to up
template<typename Float, typename S>
inline Float interpolation_weight(S low, S up, S between)
{
    const Float EPSILON = 1e-10;

    if (up >= low) {
        if (between <= low)
            return Float(0);
        if (between >= up)
            return Float(1);
        const S diff = up - low;
        const S d0 = between - low;
        if (diff < EPSILON) {
            const S d1 = up - between;
            return d0 < d1 ? Float(0) : Float(1);
        }
        return Float(d0) / Float(diff);
    } else {
        if (between >= low)
            return Float(0);
        if (between <= up)
            return Float(1);
        const S diff = low - up;
        const S d0 = low - between;
        if (diff < EPSILON) {
            const S d1 = between - up;
            return d0 < d1 ? Float(0) : Float(1);
        }
        return Float(d0) / Float(diff);
    }
}

//! return a*b-c*d
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
