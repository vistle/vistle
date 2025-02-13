#ifndef VISTLE_CORE_LINES_H
#define VISTLE_CORE_LINES_H

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

    // tubes stuff
    Vec<Scalar>::const_ptr radius() const;
    void setRadius(Vec<Scalar>::const_ptr radius);

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(CapStyle, (Open)(Flat)(Round)(Arrow))
    CapStyle startStyle() const;
    CapStyle jointStyle() const;
    CapStyle endStyle() const;
    void setCapStyles(CapStyle start, CapStyle joint, CapStyle end);

    std::set<Object::const_ptr> referencedObjects() const override;

    V_DATA_BEGIN(Lines);
    // 0: CapStyle for start, 1: CapStyle for connections within, 2: CapStyle for end of each component
    std::array<unsigned char, 3> style;
    shm_obj_ref<Vec<Scalar>> radius;

    Data(const size_t numElements = 0, const size_t numCorners = 0, const size_t numVertices = 0,
         const std::string &name = "", const Meta &meta = Meta());
    static Data *create(const std::string &name = "", const size_t numElements = 0, const size_t numCorners = 0,
                        const size_t numVertices = 0, const Meta &meta = Meta());
    V_DATA_END(Lines);

    Index cellNumFaces(Index elem) const override;
};

} // namespace vistle
#endif
