#include "vec.h"
#include "scalars.h"
#include "assert.h"
#include "indexed.h"
#include "triangles.h"
#include "celltree_impl.h"
#include "archives.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace vistle {


V_SERIALIZERS4(Vec<T,Dim>, template<class T,int Dim>);

namespace mpl = boost::mpl;

namespace {

template<int Dim>
struct instantiator {
   template <typename V> void operator()(V) {
      typedef Vec<V, Dim> VEC;
      auto vec = new VEC(0, Meta());
      vec->setSize(1);
      Index size = 0;
      auto ct = new typename Vec<V, Dim>::Celltree(size);
      
      typename VEC::Vector v;
      ct->init(&v, &v, v, v);
   }
};

template<int Dim>
struct registrator {
   template <typename S> void operator()(S) {
      typedef Vec<S,Dim> V;
      typedef typename mpl::find<Scalars, S>::type iter;
      ObjectTypeRegistry::registerType<V>(V::type());
      boost::serialization::void_cast_register<V, typename V::Base>(
            static_cast<V *>(NULL), static_cast<typename V::Base *>(NULL)
            );

   }
};

struct DoIt {
   DoIt() {
      mpl::for_each<Scalars>(registrator<1>());
      mpl::for_each<Scalars>(registrator<3>());
   }
};

static DoIt doit;

}

void inst_vecs() {

   mpl::for_each<Scalars>(instantiator<1>());
   mpl::for_each<Scalars>(instantiator<3>());
}

} // namespace vistle
