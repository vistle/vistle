#include "spheres.h"
#include "spheres_impl.h"
#include "archives.h"

namespace vistle {

Spheres::Spheres(const size_t numSpheres, const Meta &meta): Spheres::Base(Spheres::Data::create(numSpheres, meta))
{
    refreshImpl();
}

void Spheres::refreshImpl() const
{}

bool Spheres::isEmpty()
{
    return Base::isEmpty();
}

bool Spheres::isEmpty() const
{
    return Base::isEmpty();
}

bool Spheres::checkImpl() const
{
    return true;
}

Index Spheres::getNumSpheres() const
{
    return getNumCoords();
}

void Spheres::Data::initData()
{}

Spheres::Data::Data(const size_t numSpheres, const std::string &name, const Meta &meta)
: Spheres::Base::Data(numSpheres, Object::SPHERES, name, meta)
{
    initData();
}

Spheres::Data::Data(const Spheres::Data &o, const std::string &n): Spheres::Base::Data(o, n)
{
    initData();
}

Spheres::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n): Spheres::Base::Data(o, n, Object::SPHERES)
{
    initData();
}

Spheres::Data *Spheres::Data::create(const size_t numSpheres, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numSpheres, name, meta);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Spheres, Object::SPHERES)
V_OBJECT_CTOR(Spheres)
V_OBJECT_IMPL(Spheres)

} // namespace vistle
