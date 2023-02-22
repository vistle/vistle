#include "shm_array.h"
#include "shm_array_impl.h"
#include "shm_config.h"
#include "celltree.h"
#include "scalars.h"

namespace vistle {

#define SHMARR(T) \
    template class shm_array<T, shm_allocator<T>>; \
    template std::ostream &operator<< <T, shm_allocator<T>>(std::ostream &, const shm_array<T, shm_allocator<T>> &);


FOR_ALL_SCALARS(SHMARR)
SHMARR(CelltreeNode1)
SHMARR(CelltreeNode2)
SHMARR(CelltreeNode3)

} // namespace vistle
