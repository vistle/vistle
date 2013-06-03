#ifndef TRIPLE_H
#define TRIPLE_H

#define vector3 triple
#include <boost/fusion/container.hpp>
#undef triple

namespace vistle {

using boost::fusion::triple;

template<typename T1, typename T2, typename T3>
triple<T1, T2, T3> make_triple(const T1 &v1, const T2 &v2, const T3 &v3) {
   return triple<T1, T2, T3>(v1, v2, v3);
}

} // namespace vistle

#endif
