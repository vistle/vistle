#ifndef POINTS_H
#define POINTS_H


#include "coords.h"

namespace vistle {

class V_COREEXPORT Points: public Coords {
    V_OBJECT(Points);

public:
    typedef Coords Base;

    Points(const Index numPoints, const Meta &meta = Meta());

    Index getNumPoints();
    Index getNumPoints() const;

    V_DATA_BEGIN(Points);

    Data(const Index numPoints = 0, const std::string &name = "", const Meta &meta = Meta());
    Data(const Vec<Scalar, 3>::Data &o, const std::string &n);
    static Data *create(const Index numPoints = 0, const Meta &meta = Meta());

    V_DATA_END(Points);
};

} // namespace vistle
#endif
