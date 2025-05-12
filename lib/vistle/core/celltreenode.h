#ifndef VISTLE_CORE_CELLTREENODE_H
#define VISTLE_CORE_CELLTREENODE_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include "celltreenode_decl.h"

namespace vistle {

template<typename S, int NumDimensions>
struct CelltreeNode<S, 8, NumDimensions> {
    union {
        S Lmax; //< for inner nodes: max of left subvolume
        Index64 start; //< for leaf nodes: index into cell array
    };
    union {
        S Rmin; //< for inner nodes: min of right subvolume
        Index64 size; //< for leaf nodes: index into cell array
    };

    Index64 dim: NumDimensions == 1 ? 1 : 2; //< split dimension, or NumDimensions for leaf nodes
    Index64 child: sizeof(Index64) * 8 - (NumDimensions == 1 ? 1 : 2); //< index of first child for inner nodes

    CelltreeNode(Index64 start = 0, Index64 size = 0) // leaf node
    : start(start), size(size), dim(NumDimensions), child(0)
    {}

    CelltreeNode(int dim, S Lmax, S Rmin, Index64 children) // inner node
    : Lmax(Lmax), Rmin(Rmin), dim(dim), child(children)
    {}

    bool isLeaf() const { return dim == NumDimensions; }
    Index64 left() const { return child; }
    Index64 right() const { return child + 1; }

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {}
};

template<typename S, int NumDimensions>
struct CelltreeNode<S, 4, NumDimensions> {
    union {
        S Lmax; //< for inner nodes: max of left subvolume
        Index32 start; //< for leaf nodes: index into cell array
    };
    union {
        S Rmin; //< for inner nodes: min of right subvolume
        Index32 size; //< for leaf nodes: index into cell array
    };

    Index32 dim; //< split dimension, or NumDimensions for leaf nodes
    Index32 child; //< index of first child for inner nodes

    CelltreeNode(Index32 start = 0, Index32 size = 0) // leaf node
    : start(start), size(size), dim(NumDimensions), child(0)
    {}

    CelltreeNode(int dim, S Lmax, S Rmin, Index32 children) // inner node
    : Lmax(Lmax), Rmin(Rmin), dim(dim), child(children)
    {}

    bool isLeaf() const { return dim == NumDimensions; }
    Index32 left() const { return child; }
    Index32 right() const { return child + 1; }

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {}
};

template<typename S, size_t IndexSize, int NumDimensions>
bool operator<(const CelltreeNode<S, IndexSize, NumDimensions> &n0, const CelltreeNode<S, IndexSize, NumDimensions> &n1)
{
    return n0.start < n1.start;
}

template<typename S, size_t IndexSize, int NumDimensions>
bool operator>=(const CelltreeNode<S, IndexSize, NumDimensions> &n0,
                const CelltreeNode<S, IndexSize, NumDimensions> &n1)
{
    return n0.start >= n1.start;
}

template<typename S, size_t IndexSize, int NumDimensions>
bool operator!=(const CelltreeNode<S, IndexSize, NumDimensions> &n0,
                const CelltreeNode<S, IndexSize, NumDimensions> &n1)
{
    return n0.start != n1.start;
}

template<typename S, size_t IndexSize, int NumDimensions>
std::ostream &operator<<(std::ostream &os, const CelltreeNode<S, IndexSize, NumDimensions> &n)
{
    os << "(" << n.start << "+" << n.size << "/" << n.dim << " -> " << n.child << ")";
    return os;
}


#define TD_CTN_DECL(ST, SN, IS) \
    extern template struct V_COREEXPORT CelltreeNode<ST, IS / 8, 1>; \
    extern template struct V_COREEXPORT CelltreeNode<ST, IS / 8, 2>; \
    extern template struct V_COREEXPORT CelltreeNode<ST, IS / 8, 3>;

TD_CTN_DECL(float, F, 32)
TD_CTN_DECL(float, F, 64)
TD_CTN_DECL(double, D, 32)
TD_CTN_DECL(double, D, 64)

} // namespace vistle
#endif
