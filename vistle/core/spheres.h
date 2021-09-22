#ifndef SPHERES_H
#define SPHERES_H


#include "coordswradius.h"

namespace vistle {

class V_COREEXPORT Spheres: public CoordsWithRadius {
    V_OBJECT(Spheres);

public:
    typedef CoordsWithRadius Base;

    Spheres(const Index numSpheres, const Meta &meta = Meta());

    Index getNumSpheres() const;

    V_DATA_BEGIN(Spheres);
    Data(const Index numSpheres = 0, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n);
    static Data *create(const Index numSpheres = 0, const Meta &meta = Meta());

    V_DATA_END(Spheres);
};

} // namespace vistle
#endif
