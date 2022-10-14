#include "points.h"
#include "points_impl.h"
#include "archives.h"

namespace vistle {

Points::Points(const size_t numPoints, const Meta &meta): Points::Base(Points::Data::create(numPoints, meta))
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

bool Points::checkImpl() const
{
    return true;
}

void Points::print(std::ostream &os) const
{
    Base::print(os);
}

Index Points::getNumPoints()
{
    return getNumCoords();
}

Index Points::getNumPoints() const
{
    return getNumCoords();
}

void Points::Data::initData()
{}

Points::Data::Data(const size_t numPoints, const std::string &name, const Meta &meta)
: Points::Base::Data(numPoints, Object::POINTS, name, meta)
{
    initData();
}

Points::Data::Data(const Points::Data &o, const std::string &n): Points::Base::Data(o, n)
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
