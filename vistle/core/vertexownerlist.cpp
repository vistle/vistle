#include "vertexownerlist.h"

namespace vistle {

VertexOwnerList::VertexOwnerList(const Index numVertices,
      const Meta &meta)
: VertexOwnerList::Base(VertexOwnerList::Data::create(numVertices, meta))
{
}

VertexOwnerList::Data::Data(const VertexOwnerList::Data &o, const std::string &n)
: VertexOwnerList::Base::Data(o, n)
, vertexList(o.vertexList)
, cellList(o.cellList)
{
}


VertexOwnerList::Data::Data(const std::string &name, const Index numVertices,
                     const Meta &meta)
: VertexOwnerList::Base::Data(Object::Type(Object::VERTEXOWNERLIST), name, meta)
, vertexList(new ShmVector<Index>(numVertices + 1))
, cellList(new ShmVector<Index>(0))
{
}

VertexOwnerList::Data *VertexOwnerList::Data::create(const Index size,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *vol = shm<Data>::construct(name)(name, size, meta);
   publish(vol);

   return vol;
}

bool VertexOwnerList::isEmpty() const {
   return getNumVertices()==0;
}

Index VertexOwnerList::getNumVertices() const {
   return vertexList().size() - 1;
}


bool VertexOwnerList::checkImpl() const {
   V_CHECK(!vertexList().empty());
   V_CHECK(vertexList()[0] == 0);
   if (getNumVertices() > 0) {
      V_CHECK (vertexList()[getNumVertices()-1] < cellList().size());
      V_CHECK (vertexList()[getNumVertices()] == cellList().size());
   }
   return true;
}

V_OBJECT_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST);

} // namespace vistle
