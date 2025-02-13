#ifndef VISTLE_CORE_CELLTREENODE_DECL_H
#define VISTLE_CORE_CELLTREENODE_DECL_H

#include <cstdlib>
#include "index.h"

namespace vistle {

template<size_t IndexSize, int NumDimensions>
struct CelltreeNode;

typedef CelltreeNode<sizeof(Index), 1> CelltreeNode1;
typedef CelltreeNode<sizeof(Index), 2> CelltreeNode2;
typedef CelltreeNode<sizeof(Index), 3> CelltreeNode3;


} // namespace vistle
#endif
