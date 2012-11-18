#ifndef UNSTRUCTUREDGRID_H
#define UNSTRUCTUREDGRID_H


#include "shm.h"
#include "indexed.h"

namespace vistle {

class UnstructuredGrid: public Indexed {
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

   struct Info: public Base::Info {
   };

   UnstructuredGrid(const size_t numElements,
         const size_t numCorners,
         const size_t numVertices,
         const int block = -1,
         const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   shm<char>::vector &tl() const { return *(*d()->tl)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<char>::ptr tl;

      Data(const size_t numElements = 0, const size_t numCorners = 0,
                    const size_t numVertices = 0, const std::string & name = "",
                    const int block = -1, const int timestep = -1);
      static Data *create(const size_t numElements = 0,
                                    const size_t numCorners = 0,
                                    const size_t numVertices = 0,
                                    const int block = -1,
                                    const int timestep = -1);

      private:
      friend class UnstructuredGrid;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "unstr_impl.h"
#endif
#endif
