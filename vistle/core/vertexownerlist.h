#ifndef VERTEXOWNERLIST_H
#define VERTEXOWNERLIST_H

#include "export.h"
#include "scalar.h"
#include "index.h"
#include "shm.h"
#include "object.h"
#include "vector.h"

namespace vistle {

class V_COREEXPORT VertexOwnerList: public Object {
   V_OBJECT(VertexOwnerList);

 public:
   typedef Object Base;

   VertexOwnerList(const Index numVertices,
         const Meta &meta=Meta());   

   shm<Index>::array &vertexList() const { return *(*d()->vertexList)(); }
   shm<Index>::array &cellList() const { return *(*d()->cellList)(); }
   Index getNumVertices() const;
   Index getNeighbour(const Index &cell, const Index &vertex1, const Index &vertex2, const Index &vertex3) const;
   std::pair<Index*, Index> getSurroundingCells(const Index &v) const;

   V_DATA_BEGIN(VertexOwnerList);
   ShmVector<Index>::ptr vertexList;
   ShmVector<Index>::ptr cellList;

   static Data *create(const Index size = 0,
                       const Meta &m=Meta());
   Data(const std::string &name = "", const Index numVertices = 0,
        const Meta &m=Meta());
   V_DATA_END(VertexOwnerList);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "vertexownerlist_impl.h"
#endif
#endif
