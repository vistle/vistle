#ifndef TUBES_H
#define TUBES_H


#include "coordswradius.h"
#include <vistle/util/enum.h>

namespace vistle {

class V_COREEXPORT Tubes: public CoordsWithRadius {
    V_OBJECT(Tubes);

public:
    typedef CoordsWithRadius Base;

    DEFINE_ENUM_WITH_STRING_CONVERSIONS(CapStyle, (Open)(Flat)(Round)(Arrow))

    Tubes(const Index numTubes, const Index numCoords, const Meta &meta = Meta());

    Index getNumTubes() const;
    shm<Index>::array &components() { return *d()->components; }
    const shm<Index>::array &components() const { return *d()->components; }

    CapStyle startStyle() const;
    CapStyle jointStyle() const;
    CapStyle endStyle() const;

    void setCapStyles(CapStyle start, CapStyle joint, CapStyle end);

    V_DATA_BEGIN(Tubes);
    ShmVector<Index> components;
    ShmVector<unsigned char>
        style; // 0: CapStyle for start, 1: CapStyle for connections within, 2: CapStyle for end of each component

    Data(const Index numTubes = 0, const Index numCoords = 0, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n);
    static Data *create(const Index numTubes = 0, const Index numCoords = 0, const Meta &meta = Meta());

    V_DATA_END(Tubes);
};

} // namespace vistle
#endif
