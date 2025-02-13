#ifndef VISTLE_CORE_POLYGONS_H
#define VISTLE_CORE_POLYGONS_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"

namespace vistle {

class V_COREEXPORT Polygons: public Indexed {
    V_OBJECT(Polygons);

public:
    typedef Indexed Base;

    Polygons(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta = Meta());

    V_DATA_BEGIN(Polygons);
    Data(const size_t numElements, const size_t numCorners, const size_t numVertices, const std::string &name,
         const Meta &meta);
    static Data *create(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0,
                        const Meta &meta = Meta());
    V_DATA_END(Polygons);

    Index cellNumFaces(Index elem) const override;
};

} // namespace vistle
#endif
