#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <iostream>

#include "scalars.h"

#ifndef TEMPLATES_IN_HEADERS
#define VISTLE_IMPL
#endif
#include "paramvector.h"

namespace vistle {

namespace {

using namespace boost;

struct instantiator {
   template<typename S> S operator()(S) {
      typedef ParameterVector<S> V;
      S s = S();
      V *p0 = new V();
      V *p1 = new V(s);
      V *p2 = new V(S(), S());
      V *p3 = new V(S(), S(), S());
      V *p4 = new V(S(), S(), S(), S());
      V v(*p0);
      V v2;
      v2 = *p0;
      v2 = *p1;
      v2 = *p2;
      v2 = *p3;
      v2 = *p4;
      V v3(v2.dim, &v2[0]);
      V v4(v3.begin(), v3.end());

      std::cout << v << v3 << v4 << (v2==v3) << (v4 != v3);
      v = v2;
      std::cout << v.str();
      //v = v*s;
      std::cout << (V::min() < V::max());
      std::cout << (V::min() > V::max());

#if 0
      v.setMinimum(v[0], v[1], v[2], v[3]);
      v.setMinimum(&v[0]);
      v.setMaximum(v[0], v[1], v[2], v[3]);
      v.setMaximum(&v[1]);
      std::cout << v.minimum() << v.maximum() << std::endl;
#endif

      delete p0;
      delete p1;
      delete p2;
      delete p3;

      return v[0];
   }
};

} // namespace

void instantiate_vector() {

   mpl::for_each<Scalars>(instantiator());
}

} // namespace vistle
