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

template class Celltree<Scalar, Index, 1>;
template class Celltree<Scalar, Index, 2>;
template class Celltree<Scalar, Index, 3>;

V_DEFINE_SHMREF(Celltree1::Node)
V_DEFINE_SHMREF(Celltree2::Node)
V_DEFINE_SHMREF(Celltree3::Node)


static_assert(sizeof(Celltree<Scalar, Index>::Node) % 8 == 0, "bad padding");

} // namespace vistle
