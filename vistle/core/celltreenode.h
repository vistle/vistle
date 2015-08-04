#ifndef CELLTREENODE_H
#define CELLTREENODE_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include "shm.h"

namespace vistle {

template<size_t IndexSize, int NumDimensions>
struct CelltreeNode;

template<int NumDimensions>
struct CelltreeNode<8, NumDimensions> {
   union {
      struct {
         Scalar Lmax, Rmin; //< for inner nodes: max of left subvolume, min of right subvolume
      };
      struct {
         Index start, size; //< for leaf nodes: index into cell array
      };
   };

   Index dim: NumDimensions==1 ? 1 : 2; //< split dimension, or NumDimensions for leaf nodes
   Index child: sizeof(Index)*8-(NumDimensions==1 ? 1 : 2); //< index of first child for inner nodes

   CelltreeNode(Index start=0, Index size=0) // leaf node
      : start(start)
      , size(size)
      , dim(NumDimensions)
      , child(0)
      {}

   CelltreeNode(int dim, Scalar Lmax, Scalar Rmin, Index children) // inner node
      : Lmax(Lmax)
      , Rmin(Rmin)
      , dim(dim)
      , child(children)
      {}

   bool isLeaf() { return dim == NumDimensions; }
   Index left() { return child; }
   Index right() { return child+1; }

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {}
};

template<int NumDimensions>
struct CelltreeNode<4, NumDimensions> {
   union {
      struct {
         Scalar Lmax, Rmin; //< for inner nodes: max of left subvolume, min of right subvolume
      };
      struct {
         Index start, size; //< for leaf nodes: index into cell array
      };
   };

   Index dim; //< split dimension, or NumDimensions for leaf nodes
   Index child; //< index of first child for inner nodes

   CelltreeNode(Index start=0, Index size=0) // leaf node
      : start(start)
      , size(size)
      , dim(NumDimensions)
      , child(0)
      {}

   CelltreeNode(int dim, Scalar Lmax, Scalar Rmin, Index children) // inner node
      : Lmax(Lmax)
      , Rmin(Rmin)
      , dim(dim)
      , child(children)
      {}

   bool isLeaf() { return dim == NumDimensions; }
   Index left() { return child; }
   Index right() { return child+1; }

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {}
};

} // namespace vistle

#endif
