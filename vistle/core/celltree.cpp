#include "celltree.h"
#include "celltree_impl.h"
#include "vec.h"

namespace vistle {

template class Celltree<Scalar, Index, 1>;
template class Celltree<Scalar, Index, 2>;
template class Celltree<Scalar, Index, 3>;

static_assert(sizeof(Celltree<Scalar, Index>::Node) % 8 == 0, "bad padding");

} // namespace vistle
