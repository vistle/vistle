#ifndef TRIANGLES_H
#define TRIANGLES_H

#include "shm.h"
#include "coords.h"

namespace vistle {

class V_COREEXPORT Triangles: public Coords {
   V_OBJECT(Triangles);

 public:
   typedef Coords Base;

   Triangles(const size_t numCorners, const size_t numVertices,
             const Meta &meta=Meta());

   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *(*d()->cl)(); }

   V_DATA_BEGIN(Triangles);
      ShmVector<size_t>::ptr cl;

      Data(const size_t numCorners = 0, const size_t numVertices = 0,
            const std::string & name = "",
            const Meta &meta=Meta());
      static Data *create(const size_t numCorners = 0, const size_t numVertices = 0,
            const Meta &meta=Meta());
   V_DATA_END(Triangles);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "triangles_impl.h"
#endif
#endif
