#include "celltree.h"
#include "celltree_impl.h"
#include "vec.h"
#include "shm_reference_impl.h"
#include "archives.h"

namespace vistle {

#define COMMA ,
template<typename Scalar, typename Index, int NumDimensions>
V_OBJECT_IMPL_LOAD(Celltree<Scalar COMMA Index COMMA NumDimensions>)
template<typename Scalar, typename Index, int NumDimensions>
V_OBJECT_IMPL_SAVE(Celltree<Scalar COMMA Index COMMA NumDimensions>)

#define V_CELLTREE_IMPL_I(I) V_CELLTREE_I(I, , )

FOR_ALL_INDEX(V_CELLTREE_IMPL_I)

#if 0
template class Celltree<float, Index32, 1>;
template class Celltree<float, Index32, 2>;
template class Celltree<float, Index32, 3>;
template class Celltree<double, Index32, 1>;
template class Celltree<double, Index32, 2>;
template class Celltree<double, Index32, 3>;
template class Celltree<float, Index64, 1>;
template class Celltree<float, Index64, 2>;
template class Celltree<float, Index64, 3>;
template class Celltree<double, Index64, 1>;
template class Celltree<double, Index64, 2>;
template class Celltree<double, Index64, 3>;
#endif

#if 0
V_DEFINE_SHMREF(Celltree1::Node) V_DEFINE_SHMREF(Celltree2::Node) V_DEFINE_SHMREF(Celltree3::Node)
#endif

#define V_DEFINE_CT(T) V_DEFINE_SHMREF(T::Node)

    FOR_ALL_CELLTREE_TYPES(V_DEFINE_CT)


        static_assert(sizeof(Celltree<Scalar, Index>::Node) % 8 == 0, "bad padding");

} // namespace vistle
