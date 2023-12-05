#ifndef VERTEXOWNERLIST_H
#define VERTEXOWNERLIST_H

#include "export.h"
#include "index.h"
#include "object.h"
#include "archives_config.h"
#include "shmvector.h"


namespace vistle {

//! map vertices to owning elements/cells
class V_COREEXPORT VertexOwnerList: public Object {
    V_OBJECT(VertexOwnerList);

public:
    typedef Object Base;

    VertexOwnerList(const size_t numVertices, const Meta &meta = Meta());

    shm<Index>::array &vertexList() { return *d()->vertexList; }
    const ShmArrayProxy<Index> &vertexList() const { return m_vertexList; }
    shm<Index>::array &cellList() { return *d()->cellList; }
    const ShmArrayProxy<Index> &cellList() const { return m_cellList; }
    Index getNumVertices() const;
    //! return surrounding cells and their number
    std::pair<const Index *, Index> getSurroundingCells(Index v) const;

private:
    mutable ShmArrayProxy<Index> m_vertexList, m_cellList;

    V_DATA_BEGIN(VertexOwnerList);
    // index into cellList with vertex number
    ShmVector<Index> vertexList;
    // lists of cell numbers which use a vertex
    ShmVector<Index> cellList;

    static Data *create(const std::string &name = "", const size_t size = 0, const Meta &m = Meta());
    Data(const std::string &name = "", const size_t numVertices = 0, const Meta &m = Meta());
    V_DATA_END(VertexOwnerList);
};

} // namespace vistle
#endif
