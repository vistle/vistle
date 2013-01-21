#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"

namespace vistle {

class VCEXPORT UnstructuredGrid: public Indexed {
   V_OBJECT(UnstructuredGrid);

 public:
   typedef Indexed Base;

   enum Type {
      NONE        =  0,
      BAR         =  1,
      TRIANGLE    =  2,
      QUAD        =  3,
      TETRAHEDRON =  4,
      PYRAMID     =  5,
      PRISM       =  6,
      HEXAHEDRON  =  7,
      POINT       = 10,
      POLYHEDRON  = 11
   };

   UnstructuredGrid(const size_t numElements,
         const size_t numCorners,
         const size_t numVertices,
         const int block = -1,
         const int timestep = -1);

   shm<unsigned char>::vector &tl() const { return *(*d()->tl)(); }

   V_DATA_BEGIN(UnstructuredGrid);
      ShmVector<unsigned char>::ptr tl;

      Data(const size_t numElements = 0, const size_t numCorners = 0,
                    const size_t numVertices = 0, const std::string & name = "",
                    const int block = -1, const int timestep = -1);
      static Data *create(const size_t numElements = 0,
                                    const size_t numCorners = 0,
                                    const size_t numVertices = 0,
                                    const int block = -1,
                                    const int timestep = -1);

   V_DATA_END(UnstructuredGrid);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "unstr_impl.h"
#endif
#endif
