#ifndef INDEXED_H
#define INDEXED_H


#include "scalar.h"
#include "shm.h"
#include "coords.h"
#include "export.h"

namespace vistle {

class  V_COREEXPORT Indexed: public Coords {
   V_OBJECT(Indexed);

 public:
   typedef Coords Base;

   Indexed(const Index numElements, const Index numCorners,
         const Index numVertices,
         const Meta &meta=Meta());

   Index getNumElements() const;
   Index getNumCorners() const;
   Index getNumVertices() const;

   typename shm<Index>::array &cl() const { return *(*d()->cl)(); }
   typename shm<Index>::array &el() const { return *(*d()->el)(); }

   V_DATA_BEGIN(Indexed);
      ShmVector<Index>::ptr el, cl;

      Data(const Index numElements = 0, const Index numCorners = 0,
           const Index numVertices = 0,
            Type id = UNKNOWN, const std::string &name = "",
            const Meta &meta=Meta());
      static Data *create(Type id = UNKNOWN,
            const Index numElements = 0, const Index numCorners = 0,
            const Index numVertices = 0,
            const Meta &meta=Meta());

   V_DATA_END(Indexed);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "indexed_impl.h"
#endif

#endif
