#ifndef INDEXED_H
#define INDEXED_H


#include "scalar.h"
#include "shm.h"
#include "coords.h"
#include "export.h"
#include "celltree.h"
#include "vertexownerlist.h"

namespace vistle {

class V_COREEXPORT Indexed: public Coords, virtual public CelltreeInterface<3> {
    V_OBJECT(Indexed);

public:
    typedef Coords Base;
    typedef vistle::VertexOwnerList VertexOwnerList;
    typedef typename vistle::CelltreeInterface<3>::Celltree Celltree;

    Indexed(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta = Meta());

    Index getNumElements() override;
    Index getNumElements() const override;
    virtual void resetElements();
    Index getNumCorners();
    Index getNumCorners() const;
    void resetCorners();

    typename shm<Index>::array &el() { return *d()->el; }
    typename shm<Index>::array &cl() { return *d()->cl; }
    typename shm<Byte>::array &ghost() { return *d()->ghost; }
    const ShmArrayProxy<Index> &el() const { return m_el; }
    const ShmArrayProxy<Index> &cl() const { return m_cl; }
    const ShmArrayProxy<Byte> &ghost() const { return m_ghost; }
    void setGhost(Index index, bool isGhost);
    bool isGhost(Index index) const;

    std::pair<Vector3, Vector3> getBounds() const override;

    bool hasCelltree() const override;
    Celltree::const_ptr getCelltree() const override;
    bool validateCelltree() const override;

    bool hasVertexOwnerList() const;
    VertexOwnerList::const_ptr getVertexOwnerList() const;
    class V_COREEXPORT NeighborFinder {
        friend class Indexed;

    public:
        //! find neighboring element to elem containing vertices v1, v2, and v3
        Index getNeighborElement(Index elem, Index v1, Index v2, Index v3) const;
        //! return all elements containing vertex vert
        std::vector<Index> getContainingElements(Index vert) const;
        //! return all elements neighboring element
        std::vector<Index> getNeighborElements(Index elem) const;

    private:
        NeighborFinder(const Indexed *indexed);
        const Indexed *indexed;
        Index numElem, numVert;
        const Index *el, *cl;
        const Index *vl, *vol;
    };
    const NeighborFinder &getNeighborFinder() const;

    virtual std::pair<Vector3, Vector3> elementBounds(Index elem) const;
    std::vector<Index> cellVertices(Index elem) const override;
    Index cellNumFaces(Index elem) const override;
    Index cellNumVertices(Index elem) const override;

private:
    mutable Index m_numEl = InvalidIndex, m_numCl = InvalidIndex;
    mutable ShmArrayProxy<Index> m_el;
    mutable ShmArrayProxy<Index> m_cl;
    mutable ShmArrayProxy<Byte> m_ghost;
    mutable Celltree::const_ptr m_celltree;
    mutable VertexOwnerList::const_ptr m_vertexOwnerList;
    mutable std::unique_ptr<const NeighborFinder> m_neighborfinder;

    void createVertexOwnerList() const;
    void createCelltree(Index nelem, const Index *el, const Index *cl) const;

    V_DATA_BEGIN(Indexed);
    ShmVector<Index> el; //< element list: index into connectivity list - last element: sentinel
    ShmVector<Index> cl; //< connectivity list: index into coordinates
    ShmVector<Byte> ghost; //< ghost bit list: indicate if cell is ghost bit

    Data(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0, Type id = UNKNOWN,
         const std::string &name = "", const Meta &meta = Meta());
    static Data *create(const std::string &name = "", Type id = UNKNOWN, const size_t numElements = 0,
                        const size_t numCorners = 0, const size_t numVertices = 0, const Meta &meta = Meta());

    V_DATA_END(Indexed);
};

ARCHIVE_ASSUME_ABSTRACT(Indexed)
V_OBJECT_DECL(Indexed)

} // namespace vistle
#endif
