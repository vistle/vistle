#ifndef TRIANGLES_H
#define TRIANGLES_H

#include "shm.h"
#include "coords.h"

namespace vistle {

class V_COREEXPORT Triangles: public Coords {
   V_OBJECT(Triangles);

 public:
   typedef Coords Base;

   Triangles(const Index numCorners, const Index numVertices,
             const Meta &meta=Meta());

   Index getNumCorners() const;
   Index getNumVertices() const;

   typename shm<Index>::array &cl() const { return *(*d()->cl)(); }

   V_DATA_BEGIN(Triangles);
      ShmVector<Index>::ptr cl;

      Data(const Index numCorners = 0, const Index numVertices = 0,
            const std::string & name = "",
            const Meta &meta=Meta());
      static Data *create(const Index numCorners = 0, const Index numVertices = 0,
            const Meta &meta=Meta());
   V_DATA_END(Triangles);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "triangles_impl.h"
#endif
#endif
