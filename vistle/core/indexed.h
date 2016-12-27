#ifndef INDEXED_H
#define INDEXED_H


#include "scalar.h"
#include "shm.h"
#include "coords.h"
#include "export.h"
#include "celltree.h"
#include "vertexownerlist.h"

namespace vistle {

class  V_COREEXPORT Indexed: public Coords, virtual public CelltreeInterface<3> {
   V_OBJECT(Indexed);

 public:
   typedef Coords Base;
   typedef vistle::VertexOwnerList VertexOwnerList;
   typedef typename vistle::CelltreeInterface<3>::Celltree Celltree;

   Indexed(const Index numElements, const Index numCorners,
         const Index numVertices,
         const Meta &meta=Meta());

   Index getNumElements() const override;
   Index getNumCorners() const;
   Index getNumVertices() const override;

   typename shm<Index>::array &el() { return *d()->el; }
   typename shm<Index>::array &cl() { return *d()->cl; }
   const Index *el() const { return m_el; }
   const Index *cl() const { return m_cl; }

   std::pair<Vector, Vector> getBounds() const override;

   bool hasCelltree() const override;
   Celltree::const_ptr getCelltree() const override;
   bool validateCelltree() const override;

   bool hasVertexOwnerList() const;
   VertexOwnerList::const_ptr getVertexOwnerList() const;
   void removeVertexOwnerList() const;
   class NeighborFinder {
       friend class Indexed;
   public:
       //! find neighboring element to elem containing vertices v1, v2, and v3
       Index getNeighborElement(Index elem, Index v1, Index v2, Index v3);
       //! return all elements containing vertex vert
       std::vector<Index> getNeighborElements(Index vert);
   private:
       NeighborFinder(const Indexed *indexed);
       Index numElem, numVert;
       const Index *el, *cl;
       const Index *vl, *vol;
   };
   NeighborFinder getNeighborFinder() const;

   std::pair<Vector, Vector> elementBounds(Index elem) const;

 private:
   mutable const Index *m_el;
   mutable const Index *m_cl;

   void createVertexOwnerList() const;
   void createCelltree(Index nelem, const Index *el, const Index *cl) const;

   V_DATA_BEGIN(Indexed);
      ShmVector<Index> el; //< element list: index into connectivity list - last element: sentinel
      ShmVector<Index> cl; //< connectivity list: index into coordinates

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

V_OBJECT_DECLARE(vistle::Indexed);

#ifdef VISTLE_IMPL
#include "indexed_impl.h"
#endif

#endif
