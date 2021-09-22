#ifndef CELLTREENODE_H
#define CELLTREENODE_H

#include "export.h"
#include "scalar.h"
#include "index.h"
//#include "shm.h"

namespace vistle {

template<size_t IndexSize, int NumDimensions>
struct CelltreeNode;

template<int NumDimensions>
struct CelltreeNode<8, NumDimensions> {
    union {
        Scalar Lmax; //< for inner nodes: max of left subvolume
        Index start; //< for leaf nodes: index into cell array
    };
    union {
        Scalar Rmin; //< for inner nodes: min of right subvolume
        Index size; //< for leaf nodes: index into cell array
    };

    Index dim: NumDimensions == 1 ? 1 : 2; //< split dimension, or NumDimensions for leaf nodes
    Index child: sizeof(Index) * 8 - (NumDimensions == 1 ? 1 : 2); //< index of first child for inner nodes

    CelltreeNode(Index start = 0, Index size = 0) // leaf node
    : start(start), size(size), dim(NumDimensions), child(0)
    {}

    CelltreeNode(int dim, Scalar Lmax, Scalar Rmin, Index children) // inner node
    : Lmax(Lmax), Rmin(Rmin), dim(dim), child(children)
    {}

    bool isLeaf() const { return dim == NumDimensions; }
    Index left() const { return child; }
    Index right() const { return child + 1; }

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {}
};

template<int NumDimensions>
struct CelltreeNode<4, NumDimensions> {
    union {
        Scalar Lmax; //< for inner nodes: max of left subvolume
        Index start; //< for leaf nodes: index into cell array
    };
    union {
        Scalar Rmin; //< for inner nodes: min of right subvolume
        Index size; //< for leaf nodes: index into cell array
    };

    Index dim; //< split dimension, or NumDimensions for leaf nodes
    Index child; //< index of first child for inner nodes

    CelltreeNode(Index start = 0, Index size = 0) // leaf node
    : start(start), size(size), dim(NumDimensions), child(0)
    {}

    CelltreeNode(int dim, Scalar Lmax, Scalar Rmin, Index children) // inner node
    : Lmax(Lmax), Rmin(Rmin), dim(dim), child(children)
    {}

    bool isLeaf() const { return dim == NumDimensions; }
    Index left() const { return child; }
    Index right() const { return child + 1; }

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {}
};

} // namespace vistle

#endif
