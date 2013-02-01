#ifndef POLYGONS_H
#define POLYGONS_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"

namespace vistle {

class V_COREEXPORT Polygons: public Indexed {
   V_OBJECT(Polygons);

 public:
   typedef Indexed Base;

   Polygons(const size_t numElements,
            const size_t numCorners,
            const size_t numVertices,
            const int block = -1, const int timestep = -1);

   V_DATA_BEGIN(Polygons);
      Data(const size_t numElements, const size_t numCorners,
            const size_t numVertices, const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numElements = 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);
   V_DATA_END(Polygons);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "polygons_impl.h"
#endif
#endif
