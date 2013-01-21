#include <boost/mpl/vector.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/transform.hpp>
#include <iostream>

#include "scalars.h"

#define VISTLE_VECTOR_IMPL
#include "vector.h"

namespace vistle {

namespace {

using namespace boost;

struct instantiator {
   template<typename S> S operator()(S) {
      typedef ParameterVector<S> V;
      S s = S();
      V *p0 = new V();
      V *p1 = new V(S());
      V *p2 = new V(S(), S());
      V *p3 = new V(S(), S(), S());
      V *p4 = new V(S(), S(), S(), S());
      V v(*p0);
      V v2 = *p0 + *p1 - *p2 + *p3 + *p4 + v;
      V v3(v2.dim, &v2[0]);
      V v4(v3.begin(), v3.end());

      std::cout << v << v3 << v4;
      v = v2;
      std::cout << v.str();
      v = v*s;

      return v*v2;
   }
};

} // namespace

void instantiate_vector() {

   mpl::for_each<Scalars>(instantiator());
}

} // namespace vistle
