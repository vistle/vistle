#ifndef LINES_H
#define LINES_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Lines: public Indexed {
    V_OBJECT(Lines);

public:
    typedef Indexed Base;

    Lines(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta = Meta());

    // creates a line between two points
    void AddLine(std::array<Scalar, 3> point1, std::array<Scalar, 3> point2);

    V_DATA_BEGIN(Lines);
    Data(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0,
         const std::string &name = "", const Meta &meta = Meta());
    static Data *create(const std::string &name = "", const size_t numElements = 0, const size_t numCorners = 0,
                        const size_t numVertices = 0, const Meta &meta = Meta());
    V_DATA_END(Lines);
};

} // namespace vistle
#endif
