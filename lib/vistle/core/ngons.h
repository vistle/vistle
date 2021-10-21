#ifndef VISTLE_NGONS_H
#define VISTLE_NGONS_H

#include "shm.h"
#include "coords.h"
#include "geometry.h"
#include "celltree.h"

namespace vistle {

template<int N>
class V_COREEXPORT Ngons: public Coords, virtual public CelltreeInterface<3> {
    V_OBJECT(Ngons);

public:
    static constexpr int num_corners = N;
    typedef Coords Base;
    typedef typename vistle::CelltreeInterface<3>::Celltree Celltree;

    Ngons(const Index numCorners, const Index numCoords, const Meta &meta = Meta());

    Index getNumElements() override;
    Index getNumElements() const override;
    Index getNumCorners();
    Index getNumCorners() const;

    shm<Index>::array &cl() { return *d()->cl; }
    const Index *cl() const { return m_cl; }

    bool hasCelltree() const override;
    Celltree::const_ptr getCelltree() const override;
    bool validateCelltree() const override;

    virtual std::pair<Vector, Vector> elementBounds(Index elem) const;

    Index cellNumFaces(Index elem) const override { return 1; }
    std::vector<Index> cellVertices(Index elem) const override;

private:
    void createCelltree(Index nelem, const Index *cl) const;

    mutable const Index *m_cl;
    mutable Index m_numCorners = 0;
    mutable Celltree::const_ptr m_celltree;

    V_DATA_BEGIN(Ngons);
    ShmVector<Index> cl;

    Data(const Index numCorners = 0, const Index numCoords = 0, const std::string &name = "",
         const Meta &meta = Meta());
    static Data *create(const Index numCorners = 0, const Index numCoords = 0, const Meta &meta = Meta());
    V_DATA_END(Ngons);

    static_assert(N == 3 || N == 4, "only usable for triangles and quads");
};

} // namespace vistle
#endif
