#ifndef VISTLE_CORE_CELLTREENODE_DECL_H
#define VISTLE_CORE_CELLTREENODE_DECL_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/copy.hpp>

namespace vistle {

template<typename S, size_t IndexSize, int NumDimensions>
struct CelltreeNode;

typedef CelltreeNode<Scalar, sizeof(Index), 1> CelltreeNode1;
typedef CelltreeNode<Scalar, sizeof(Index), 2> CelltreeNode2;
typedef CelltreeNode<Scalar, sizeof(Index), 3> CelltreeNode3;

#define TD_CTN(ST, SN, IS) \
    typedef CelltreeNode<ST, IS / 8, 1> CelltreeNode##IS##SN##1; \
    typedef CelltreeNode<ST, IS / 8, 2> CelltreeNode##IS##SN##2; \
    typedef CelltreeNode<ST, IS / 8, 3> CelltreeNode##IS##SN##3;

TD_CTN(float, F, 32)
TD_CTN(float, F, 64)
TD_CTN(double, D, 32)
TD_CTN(double, D, 64)

#define FOR_ALL_CELLTREE_NODES(MACRO) \
    MACRO(CelltreeNode32F1) \
    MACRO(CelltreeNode32F2) \
    MACRO(CelltreeNode32F3) \
    MACRO(CelltreeNode32D1) \
    MACRO(CelltreeNode32D2) \
    MACRO(CelltreeNode32D3) \
    MACRO(CelltreeNode64F1) \
    MACRO(CelltreeNode64F2) \
    MACRO(CelltreeNode64F3) \
    MACRO(CelltreeNode64D1) \
    MACRO(CelltreeNode64D2) \
    MACRO(CelltreeNode64D3)

#define CELLTREE_TYPES(D) \
    typedef boost::mpl::vector<CelltreeNode32F##D, CelltreeNode64F##D, CelltreeNode32D##D, CelltreeNode64D##D> \
        CelltreeNode##D##Types;
CELLTREE_TYPES(1);
CELLTREE_TYPES(2);
CELLTREE_TYPES(3);

typedef boost::mpl::copy<CelltreeNode1Types, boost::mpl::back_inserter<CelltreeNode2Types>>::type CelltreeNode12Types;
typedef boost::mpl::copy<CelltreeNode12Types, boost::mpl::back_inserter<CelltreeNode3Types>>::type CelltreeNode123Types;
typedef CelltreeNode123Types CelltreeNodeTypes;

} // namespace vistle
#endif
