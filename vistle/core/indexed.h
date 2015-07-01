#ifndef INDEXED_H
#define INDEXED_H


#include "scalar.h"
#include "shm.h"
#include "coords.h"
#include "export.h"
#include "celltree.h"
#include "vertexownerlist.h"

namespace vistle {

class  V_COREEXPORT Indexed: public Coords {
   V_OBJECT(Indexed);

 public:
   typedef Coords Base;
   typedef vistle::VertexOwnerList VertexOwnerList;

   Indexed(const Index numElements, const Index numCorners,
         const Index numVertices,
         const Meta &meta=Meta());

   Index getNumElements() const;
   Index getNumCorners() const;
   Index getNumVertices() const;

   typename shm<Index>::array &el() const { return *(*d()->el)(); }
   typename shm<Index>::array &cl() const { return *(*d()->cl)(); }


   Celltree::const_ptr getCelltree() const;
   bool validateCelltree() const;

   bool hasVertexOwnerList() const;
   VertexOwnerList::const_ptr getVertexOwnerList() const;
   void removeVertexOwnerList() const;

   bool getElementBounds(Index elem, Vector *min, Vector *max) const;

 private:
   void createVertexOwnerList() const;

   V_DATA_BEGIN(Indexed);
      ShmVector<Index>::ptr el; //< element list: index into connectivity list - last element: sentinel
      ShmVector<Index>::ptr cl; //< connectivity list: index into coordinates

      Data(const Index numElements = 0, const Index numCorners = 0,
           const Index numVertices = 0,
            Type id = UNKNOWN, const std::string &name = "",
            const Meta &meta=Meta());
      static Data *create(const std::string &name="", Type id = UNKNOWN,
            const Index numElements = 0, const Index numCorners = 0,
            const Index numVertices = 0,
            const Meta &meta=Meta());

   V_DATA_END(Indexed);
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Indexed)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "indexed_impl.h"
#endif

#endif
