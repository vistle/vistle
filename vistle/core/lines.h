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

    Lines(const Index numElements, const Index numCorners, const Index numVertices, const Meta &meta = Meta());

    V_DATA_BEGIN(Lines);
    Data(const Index numElements = 0, const Index numCorners = 0, const Index numVertices = 0,
         const std::string &name = "", const Meta &meta = Meta());
    static Data *create(const std::string &name = "", const Index numElements = 0, const Index numCorners = 0,
                        const Index numVertices = 0, const Meta &meta = Meta());
    V_DATA_END(Lines);
};

} // namespace vistle
#endif
