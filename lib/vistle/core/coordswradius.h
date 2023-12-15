#ifndef COORDSWITHRADIUS_H
#define COORDSWITHRADIUS_H


#include "coords.h"

namespace vistle {

class V_COREEXPORT CoordsWithRadius: public Coords {
    V_OBJECT(CoordsWithRadius);

public:
    typedef Coords Base;

    CoordsWithRadius(const size_t numCoords, const Meta &meta = Meta());

    shm<Scalar>::array &r() { return *(d()->r); }
    const ShmArrayProxy<Scalar> &r() const { return m_r; }
    void resetArrays() override;
    void setSize(const size_t size) override;

private:
    mutable ShmArrayProxy<Scalar> m_r;

    V_DATA_BEGIN(CoordsWithRadius);
    ShmVector<Scalar> r;

    Data(const size_t numCoords = 0, Type id = UNKNOWN, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id);
    static Data *create(const std::string &name = "", Type id = UNKNOWN, const size_t numCoords = 0,
                        const Meta &meta = Meta());

    V_DATA_END(CoordsWithRadius);
};

V_OBJECT_DECL(CoordsWithRadius)

//ARCHIVE_ASSUME_ABSTRACT(CoordsWithRadius)

} // namespace vistle
#endif
