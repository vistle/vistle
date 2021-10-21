#ifndef POLYGONS_H
#define POLYGONS_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"

namespace vistle {

class V_COREEXPORT Polygons: public Indexed {
    V_OBJECT(Polygons);

public:
    typedef Indexed Base;

    Polygons(const Index numElements, const Index numCorners, const Index numVertices, const Meta &meta = Meta());

    V_DATA_BEGIN(Polygons);
    Data(const Index numElements, const Index numCorners, const Index numVertices, const std::string &name,
         const Meta &meta);
    static Data *create(const Index numElements = 0, const Index numCorners = 0, const Index numVertices = 0,
                        const Meta &meta = Meta());
    V_DATA_END(Polygons);
};

} // namespace vistle
#endif
