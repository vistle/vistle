#ifndef VISTLE_MATH_H
#define VISTLE_MATH_H

#include <algorithm>

namespace vistle {

template<typename S>
S clamp(S v, S vmin, S vmax) {
   return std::min(std::max(v, vmin), vmax);
}

template<typename S, typename Float>
inline S lerp(S a, S b, Float t) {
    return a+t*(b-a);
}

}
#endif
