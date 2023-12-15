#include "vertexownerlist.h"
#include "vertexownerlist_impl.h"
#include "archives.h"

namespace vistle {

VertexOwnerList::VertexOwnerList(const size_t numVertices, const Meta &meta)
: VertexOwnerList::Base(VertexOwnerList::Data::create("", numVertices, meta))
{
    refreshImpl();
}

void VertexOwnerList::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    if (d) {
        m_vertexList = d->vertexList;
        m_cellList = d->cellList;
    } else {
        m_vertexList = nullptr;
        m_cellList = nullptr;
    }
}

void VertexOwnerList::Data::initData()
{
    cellList.construct(0);
}

VertexOwnerList::Data::Data(const VertexOwnerList::Data &o, const std::string &n)
: VertexOwnerList::Base::Data(o, n), vertexList(o.vertexList), cellList(o.cellList)
{}


VertexOwnerList::Data::Data(const std::string &name, const size_t numVertices, const Meta &meta)
: VertexOwnerList::Base::Data(Object::Type(Object::VERTEXOWNERLIST), name, meta)
{
    initData();
    vertexList.construct(numVertices + 1);
}

VertexOwnerList::Data *VertexOwnerList::Data::create(const std::string &objId, const size_t size, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *vol = shm<Data>::construct(name)(name, size, meta);
    publish(vol);

    return vol;
}

bool VertexOwnerList::isEmpty()
{
    return getNumVertices() == 0;
}

bool VertexOwnerList::isEmpty() const
{
    return getNumVertices() == 0;
}

void VertexOwnerList::print(std::ostream &os) const
{
    Base::print(os);
    os << " vertexlist(" << *d()->vertexList << ")";
    os << " celllist(" << *d()->cellList << ")";
}

Index VertexOwnerList::getNumVertices() const
{
    return d()->vertexList->size() - 1;
}

std::pair<const Index *, Index> VertexOwnerList::getSurroundingCells(Index v) const
{
    Index start = m_vertexList[v];
    Index end = m_vertexList[v + 1];
    const Index *ptr = &m_cellList[start];
    Index n = end - start;
    return std::make_pair(ptr, n);
}

bool VertexOwnerList::checkImpl() const
{
    CHECK_OVERFLOW(d()->vertexList->size());

    V_CHECK(!d()->vertexList->empty());
    V_CHECK(d()->vertexList->at(0) == 0);
    if (getNumVertices() > 0) {
        V_CHECK(d()->vertexList->at(getNumVertices() - 1) < d()->cellList->size());
        V_CHECK(d()->vertexList->at(getNumVertices()) == d()->cellList->size());
    }
    return true;
}

V_OBJECT_TYPE(VertexOwnerList, Object::VERTEXOWNERLIST)
V_OBJECT_CTOR(VertexOwnerList)
V_OBJECT_IMPL(VertexOwnerList)

} // namespace vistle
