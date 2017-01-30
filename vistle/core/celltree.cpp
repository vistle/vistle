#include "celltree.h"
#include "archives.h"

#include "celltree_impl.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace mpl = boost::mpl;

namespace vistle {

		typedef Celltree<Scalar, Index, 1> Celltree1; // should have already been typedefed in celltree.h but 
		typedef Celltree<Scalar, Index, 2> Celltree2; // that does not work with VS2015
		typedef Celltree<Scalar, Index, 3> Celltree3; // it does work like this however
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

}

void inst_celltrees() {

   mpl::for_each<Scalars>(instantiator<1>());
   mpl::for_each<Scalars>(instantiator<2>());
   mpl::for_each<Scalars>(instantiator<3>());
}

} // namespace vistle
