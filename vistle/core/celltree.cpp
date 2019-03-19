#include "celltree.h"
#include "celltree_impl.h"
#include "vec.h"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/find.hpp>

namespace mpl = boost::mpl;

namespace vistle {

template class V_COREEXPORT Celltree<Scalar, Index, 1>;
template class V_COREEXPORT Celltree<Scalar, Index, 2>;
template class V_COREEXPORT Celltree<Scalar, Index, 3>;

} // namespace vistle
