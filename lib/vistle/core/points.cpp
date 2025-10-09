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
