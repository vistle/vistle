#include "archives.h"
#include "archives_config.h"
#include "celltreenode.h"

namespace vistle {

#define TD_CTN_IMPL(ST, SN, IS) \
    template struct CelltreeNode<ST, IS / 8, 1>; \
    template struct CelltreeNode<ST, IS / 8, 2>; \
    template struct CelltreeNode<ST, IS / 8, 3>;

TD_CTN_IMPL(float, F, 32)
TD_CTN_IMPL(float, F, 64)
TD_CTN_IMPL(double, D, 32)
TD_CTN_IMPL(double, D, 64)

} // namespace vistle
