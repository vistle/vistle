#include "polygons.h"
#include "polygons_impl.h"
#include "archives.h"

namespace vistle {

Polygons::Polygons(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta)
: Polygons::Base(Polygons::Data::create(numElements, numCorners, numVertices, meta))
{
    refreshImpl();
}

bool Polygons::isEmpty()
{
    return Base::isEmpty();
}

bool Polygons::isEmpty() const
{
    return Base::isEmpty();
}

void Polygons::refreshImpl() const
{}

bool Polygons::checkImpl() const
{
    return true;
}

void Polygons::print(std::ostream &os) const
{
    Base::print(os);
}

void Polygons::Data::initData()
{}

Polygons::Data::Data(const Polygons::Data &o, const std::string &n): Polygons::Base::Data(o, n)
{
    initData();
}

Polygons::Data::Data(const size_t numElements, const size_t numCorners, const size_t numVertices,
                     const std::string &name, const Meta &meta)
: Polygons::Base::Data(numElements, numCorners, numVertices, Object::POLYGONS, name, meta)
{
    initData();
}


Polygons::Data *Polygons::Data::create(const size_t numElements, const size_t numCorners, const size_t numVertices,
                                       const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Polygons, Object::POLYGONS)
V_OBJECT_CTOR(Polygons)
V_OBJECT_IMPL(Polygons)

} // namespace vistle
