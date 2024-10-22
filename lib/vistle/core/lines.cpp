#include "lines.h"
#include "lines_impl.h"
#include "archives.h"
#include "validate.h"

namespace vistle {

Lines::Lines(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta)
: Lines::Base(Lines::Data::create("", numElements, numCorners, numVertices, meta))
{
    refreshImpl();
}

void Lines::refreshImpl() const
{}

bool Lines::isEmpty()
{
    return Base::isEmpty();
}

bool Lines::isEmpty() const
{
    return Base::isEmpty();
}

bool Lines::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_SUB(radius());
    return true;
}

void Lines::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    os << " radius(";
    if (radius()) {
        radius()->print(os, verbose);
    }
    os << ")";
}

Vec<Scalar>::const_ptr Lines::radius() const
{
    return d()->radius.getObject();
}

void Lines::setRadius(Vec<Scalar>::const_ptr radius)
{
    assert(!radius || radius->check(std::cerr));
    d()->radius = radius;
}

std::set<Object::const_ptr> Lines::referencedObjects() const
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

Lines::CapStyle Lines::startStyle() const
{
    return (CapStyle)d()->style[0];
}

Lines::CapStyle Lines::jointStyle() const
{
    return (CapStyle)d()->style[1];
}

Lines::CapStyle Lines::endStyle() const
{
    return (CapStyle)d()->style[2];
}

void Lines::setCapStyles(Lines::CapStyle start, Lines::CapStyle joint, Lines::CapStyle end)
{
    d()->style[0] = start;
    d()->style[1] = joint;
    d()->style[2] = end;
}

Index Lines::cellNumFaces(Index elem) const
{
    return 0;
}

void Lines::Data::initData()
{
    style[0] = style[1] = style[2] = Lines::Open;
}


Lines::Data::Data(const Data &other, const std::string &name): Lines::Base::Data(other, name), radius(other.radius)
{
    initData();
    std::copy(other.style.begin(), other.style.end(), style.begin());
}

Lines::Data::Data(const size_t numElements, const size_t numCorners, const size_t numVertices, const std::string &name,
                  const Meta &meta)
: Lines::Base::Data(numElements, numCorners, numVertices, Object::LINES, name, meta)
{
    initData();
}


Lines::Data *Lines::Data::create(const std::string &objId, const size_t numElements, const size_t numCorners,
                                 const size_t numVertices, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
    publish(l);

    return l;
}

V_OBJECT_TYPE(Lines, Object::LINES)
V_OBJECT_CTOR(Lines)
V_OBJECT_IMPL(Lines)

} // namespace vistle
