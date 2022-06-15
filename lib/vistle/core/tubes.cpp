#include "tubes.h"
#include "tubes_impl.h"
#include "archives.h"

namespace vistle {

Tubes::Tubes(const size_t numTubes, const size_t numCoords, const Meta &meta)
: Tubes::Base(Tubes::Data::create(numTubes, numCoords, meta))
{
    refreshImpl();
}

void Tubes::refreshImpl() const
{}

bool Tubes::isEmpty()
{
    return Base::isEmpty();
}

bool Tubes::isEmpty() const
{
    return Base::isEmpty();
}

bool Tubes::checkImpl() const
{
    CHECK_OVERFLOW(d()->components->size());

    V_CHECK(components().size() > 0);
    V_CHECK(components()[0] == 0);
    V_CHECK(getNumTubes() >= 0);
    V_CHECK(components()[getNumTubes()] <= getNumVertices());
    V_CHECK(getNumVertices() >= getNumTubes());
    return true;
}

Index Tubes::getNumTubes() const
{
    return components().size() - 1;
}

Tubes::CapStyle Tubes::startStyle() const
{
    return (CapStyle)d()->style[0];
}

Tubes::CapStyle Tubes::jointStyle() const
{
    return (CapStyle)d()->style[1];
}

Tubes::CapStyle Tubes::endStyle() const
{
    return (CapStyle)d()->style[2];
}

void Tubes::setCapStyles(Tubes::CapStyle start, Tubes::CapStyle joint, Tubes::CapStyle end)
{
    d()->style[0] = start;
    d()->style[1] = joint;
    d()->style[2] = end;
}

void Tubes::Data::initData()
{
    style[0] = style[1] = style[2] = Tubes::Open;
}

Tubes::Data::Data(const size_t numTubes, const size_t numCoords, const std::string &name, const Meta &meta)
: Tubes::Base::Data(numCoords, Object::TUBES, name, meta)
{
    CHECK_OVERFLOW(numTubes + 1);
    initData();
    components.construct(numTubes + 1);
    (*components)[0] = 0;
}

Tubes::Data::Data(const Tubes::Data &o, const std::string &n): Tubes::Base::Data(o, n), components(o.components)
{
    initData();
    for (size_t i = 0; i < style.size(); ++i)
        style[i] = o.style[i];
}

Tubes::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n): Tubes::Base::Data(o, n, Object::TUBES)
{
    initData();
    components.construct(1);
    (*components)[0] = 0;
}

Tubes::Data *Tubes::Data::create(const size_t numTubes, const size_t numCoords, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numTubes, numCoords, name, meta);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Tubes, Object::TUBES)
V_OBJECT_CTOR(Tubes)
V_OBJECT_IMPL(Tubes)

} // namespace vistle
