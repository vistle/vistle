#include "normals.h"
#include "normals_impl.h"
#include "archives.h"

namespace vistle {

Normals::Normals(const Index numNormals, const Meta &meta): Normals::Base(Normals::Data::create(numNormals, meta))
{
    refreshImpl();
}

void Normals::refreshImpl() const
{}

bool Normals::isEmpty()
{
    return Base::isEmpty();
}

bool Normals::isEmpty() const
{
    return Base::isEmpty();
}

bool Normals::checkImpl() const
{
    return true;
}

Index Normals::getNumNormals() const
{
    return getSize();
}

void Normals::Data::initData()
{}

Normals::Data::Data(const Index numNormals, const std::string &name, const Meta &meta)
: Normals::Base::Data(numNormals, Object::NORMALS, name, meta)
{
    initData();
}

Normals::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n): Normals::Base::Data(o, n, Object::NORMALS)
{
    initData();
}

Normals::Data::Data(const Normals::Data &o, const std::string &n): Normals::Base::Data(o, n)
{
    initData();
}

Normals::Data *Normals::Data::create(const Index numNormals, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId();
    Data *p = shm<Data>::construct(name)(numNormals, name, meta);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Normals, Object::NORMALS)
V_OBJECT_CTOR(Normals)
V_OBJECT_IMPL(Normals)

} // namespace vistle
