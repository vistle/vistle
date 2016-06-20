#include "vertexownerlist.h"
#include "archives.h"

namespace vistle {

VertexOwnerList::VertexOwnerList(const Index numVertices,
      const Meta &meta)
: VertexOwnerList::Base(VertexOwnerList::Data::create("", numVertices, meta))
{
    refreshImpl();
}

void VertexOwnerList::refreshImpl() const {
    const Data *d = static_cast<Data *>(m_data);
    m_vertexList = (d && d->vertexList.valid()) ? d->vertexList->data() : nullptr;
    m_cellList = (d && d->cellList.valid()) ? d->cellList->data() : nullptr;
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
{
    vertexList.construct(numVertices+1);
    cellList.construct(0);
}

VertexOwnerList::Data *VertexOwnerList::Data::create(const std::string &objId, const Index size,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *vol = shm<Data>::construct(name)(name, size, meta);
   publish(vol);

   return vol;
}

bool VertexOwnerList::isEmpty() const {
   return getNumVertices()==0;
}

Index VertexOwnerList::getNumVertices() const {
   return d()->vertexList->size() - 1;
}

std::pair<const Index*,Index> VertexOwnerList::getSurroundingCells(Index v) const {
   Index start=m_vertexList[v];
   Index end=m_vertexList[v+1];
   const Index* ptr = &m_cellList[start];
   Index n=end-start;
   return std::make_pair(ptr, n);
}

Index VertexOwnerList::getNeighbour(Index cell, Index vertex1, Index vertex2, Index vertex3) const {
   auto vertexList=m_vertexList;
   auto cellList=m_cellList;
   std::map<Index,Index> cellCount;
   std::vector<Index> vertices = {vertex1, vertex2, vertex3};

   if (vertex1 == vertex2 || vertex1 == vertex3 || vertex2 == vertex3) {
      std::cerr << "WARNING: getNeighbour was not called with 3 unique vertices." << std::endl;
   }

   for (Index i=0; i<3; ++i) {
      for (Index j=vertexList[vertices[i]]; j<vertexList[vertices[i] + 1]; ++j) {
         Index cell=cellList[j];
         ++cellCount[cell];
      }
   }

   for (auto &c: cellCount) {
      if (c.second == 3) {
         if (c.first != cell) {
            return c.first;
         }
      }
   }

   return InvalidIndex;
}

bool VertexOwnerList::checkImpl() const {
   V_CHECK(!d()->vertexList->empty());
   V_CHECK(d()->vertexList->at(0) == 0);
   if (getNumVertices() > 0) {
      V_CHECK (d()->vertexList->at(getNumVertices()-1) < d()->cellList->size());
      V_CHECK (d()->vertexList->at(getNumVertices()) == d()->cellList->size());
   }
   return true;
}

V_OBJECT_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST);
V_OBJECT_CTOR(VertexOwnerList);

} // namespace vistle
