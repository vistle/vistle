#include "points.h"
#include "points_impl.h"
#include "archives.h"
#include "validate.h"
#include "shm_obj_ref_impl.h"

namespace vistle {

Points::Points(size_t numPoints, const Meta &meta): Points::Base(Points::Data::create(numPoints, meta))
{
    refreshImpl();
}

bool Points::isEmpty()
{
    return Base::isEmpty();
}

bool Points::isEmpty() const
{
    return Base::isEmpty();
}

void Points::refreshImpl() const
{}

bool Points::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_SUB(radius());
    return true;
}

void Points::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    os << " radius(";
    if (radius()) {
        radius()->print(os, verbose);
    }
    os << ")";
}

Index Points::getNumPoints()
{
    return getNumCoords();
}

Index Points::getNumPoints() const
{
    return getNumCoords();
}

Vec<Scalar>::const_ptr Points::radius() const
{
    return d()->radius.getObject();
}

void Points::setRadius(Vec<Scalar>::const_ptr radius)
{
    d()->radius = radius;
}

std::pair<vistle::Vector3, vistle::Vector3> Points::getBounds() const
{
    auto rad = radius();
    if (!rad) {
        return Base::getBounds();
    }

    const Scalar *r = rad->x().data();

    Vector3 min;
    Vector3 max;
    const auto numPoints = getNumPoints();

    std::array<const Scalar *, 3> coords;
    for (unsigned c = 0; c < 3; ++c) {
        min[c] = std::numeric_limits<Scalar>::max();
        max[c] = std::numeric_limits<Scalar>::lowest();
        coords[c] = this->x(c).data();
    }

    for (Index i = 0; i < numPoints; ++i) {
        for (unsigned c = 0; c < 3; ++c) {
            if (coords[c][i] - r[i] < min[c])
                min[c] = coords[c][i] - r[i];
            if (coords[c][i] + r[i] > max[c])
                max[c] = coords[c][i] + r[i];
        }
    }

    return {min, max};
}

Index Points::getNumElements()
{
    return getNumPoints();
}

Index Points::getNumElements() const
{
    return getNumPoints();
}

Index Points::cellNumFaces(Index elem) const
{
    return 0;
}

Index Points::cellNumVertices(Index elem) const
{
    return 1;
}

std::vector<Index> Points::cellVertices(Index elem) const
{
    return std::vector<Index>(1, elem);
}

Vector3 Points::cellCenter(Index elem) const
{
    const Scalar *x = this->x().data();
    const Scalar *y = this->y().data();
    const Scalar *z = this->z().data();
    return Vector3(x[elem], y[elem], z[elem]);
}

std::set<Object::const_ptr> Points::referencedObjects() const
{
    auto objs = Base::referencedObjects();

    auto rad = radius();
    if (rad && objs.emplace(rad).second) {
        auto ro = rad->referencedObjects();
        std::copy(ro.begin(), ro.end(), std::inserter(objs, objs.begin()));
        objs.emplace(rad);
    }

    return objs;
}

void Points::Data::initData()
{}

Points::Data::Data(const size_t numPoints, const std::string &name, const Meta &meta)
: Points::Base::Data(numPoints, Object::POINTS, name, meta)
{
    initData();
}

Points::Data::Data(const Points::Data &o, const std::string &n): Points::Base::Data(o, n), radius(o.radius)
{
    initData();
}

Points::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n): Points::Base::Data(o, n, Object::POINTS)
{
    initData();
}

Points::Data *Points::Data::create(const size_t numPoints, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numPoints, name, meta);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Points, Object::POINTS)
V_OBJECT_CTOR(Points)
V_OBJECT_IMPL(Points)

} // namespace vistle
