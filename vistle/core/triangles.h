#ifndef TRIANGLES_H
#define TRIANGLES_H

#include "shm.h"
#include "coords.h"
#include "geometry.h"

namespace vistle {

class V_COREEXPORT Triangles: public Coords, virtual public ElementInterface {
   V_OBJECT(Triangles);

 public:
   typedef Coords Base;

   Triangles(const Index numCorners, const Index numCoords,
             const Meta &meta=Meta());

   Index getNumElements() override;
   Index getNumElements() const override;
   Index getNumCorners();
   Index getNumCorners() const;

   shm<Index>::array &cl() { return *d()->cl; }
   const Index *cl() const { return m_cl; }

 private:
   mutable const Index *m_cl;
   mutable Index m_numCorners = 0;

   V_DATA_BEGIN(Triangles);
      ShmVector<Index> cl;

      Data(const Index numCorners = 0, const Index numCoords = 0,
            const std::string & name = "",
            const Meta &meta=Meta());
      static Data *create(const Index numCorners = 0, const Index numCoords = 0,
            const Meta &meta=Meta());
   V_DATA_END(Triangles);
};

} // namespace vistle
#endif

#ifdef VISTLE_IMPL
#include "triangles_impl.h"
#endif
