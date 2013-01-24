#ifndef COORDS_H
#define COORDS_H

#include "scalar.h"
#include "shm.h"
#include "object.h"
#include "vec.h"
#include "export.h"

namespace vistle {

class VCEXPORT Coords: public Vec<Scalar,3> {
   V_OBJECT(Coords);

 public:
   typedef Vec<Scalar,3> Base;

   Coords(const size_t numVertices,
             const int block = -1, const int timestep = -1);

   size_t getNumCoords() const;
   size_t getNumVertices() const;

   V_DATA_BEGIN(Coords);
      Data(const size_t numVertices = 0,
            Type id = UNKNOWN, const std::string & name = "",
            const int block = -1, const int timestep = -1);
      static Data *create(Type id = UNKNOWN, const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

   V_DATA_END(Coords);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "coords_impl.h"
#endif
#endif
