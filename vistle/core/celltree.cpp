#include "celltree.h"
#include "archives.h"

#include "celltree_impl.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace mpl = boost::mpl;

namespace vistle {

V_OBJECT_TYPE_T(Celltree1, Object::CELLTREE1);
V_OBJECT_TYPE_T(Celltree2, Object::CELLTREE2);
V_OBJECT_TYPE_T(Celltree3, Object::CELLTREE3);


namespace {

template<int Dim>
struct instantiator {
   template <typename V> void operator()(V) {

      Index size = 0;
      auto ct = new typename CelltreeInterface<Dim>::Celltree(size);

      ScalarVector<Dim> v;
      ct->init(&v, &v, v, v);
   }
};

#if 0
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
#endif

}

void inst_celltrees() {

   mpl::for_each<Scalars>(instantiator<1>());
   mpl::for_each<Scalars>(instantiator<2>());
   mpl::for_each<Scalars>(instantiator<3>());
}

} // namespace vistle
