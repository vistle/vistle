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
    m_vertexList = (d && d->vertexList.valid()) ? d->vertexList->data() : nullptr;
    m_cellList = (d && d->cellList.valid()) ? d->cellList->data() : nullptr;
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

Index VertexOwnerList::getNeighbor(Index cell, Index vertex1, Index vertex2, Index vertex3) const
{
    std::map<Index, Index> cellCount;
    std::array<Index, 3> vertices;

    if (vertex1 == vertex2 || vertex1 == vertex3 || vertex2 == vertex3) {
        std::cerr << "WARNING: getNeighbor was not called with 3 unique vertices." << std::endl;
        return InvalidIndex;
    }
    vertices[0] = vertex1;
    vertices[1] = vertex2;
    vertices[2] = vertex3;

    for (Index i = 0; i < 3; ++i) {
        const auto cells_num = getSurroundingCells(vertices[i]);
        auto cells = cells_num.first;
        auto num = cells_num.second;
        for (Index j = 0; j < num; ++j) {
            Index c = cells[j];
            if (c != cell)
                ++cellCount[c];
        }
    }

    for (auto &c: cellCount) {
        if (c.second == 3) {
            return c.first;
        }
    }

    return InvalidIndex;
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
