#include "coords.h"
#include "archives.h"

#include "coords_impl.h"

namespace vistle {

Coords::Coords(const Index numVertices, const Meta &meta): Coords::Base(static_cast<Data *>(NULL))
{
    refreshImpl();
}

void Coords::resetCoords()
{
    resetArrays();
}

void Coords::refreshImpl() const
{}

bool Coords::isEmpty()
{
    return Base::isEmpty();
}

bool Coords::isEmpty() const
{
    return Base::isEmpty();
}

bool Coords::checkImpl() const
{
    V_CHECK(!normals() || normals()->getNumNormals() == getSize());
    if (normals()) {
        V_CHECK(normals()->check());
    }
    return true;
}

std::pair<Vector3, Vector3> Coords::getBounds() const
{
    return getMinMax();
}

void Coords::Data::initData()
{}

Coords::Data::Data(const Index numVertices, Type id, const std::string &name, const Meta &meta)
: Coords::Base::Data(numVertices, id, name, meta)
{
    initData();
}

Coords::Data::Data(const Coords::Data &o, const std::string &n): Coords::Base::Data(o, n), normals(o.normals)
{
    initData();
}

Coords::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id): Coords::Base::Data(o, n, id)
{
    initData();
}

Coords::Data *Coords::Data::create(const std::string &objId, Type id, const Index numVertices, const Meta &meta)
{
    assert("should never be called" == NULL);

    return NULL;
}

Coords::Data::~Data()
{}

Index Coords::getNumVertices() const
{
    return getSize();
}

Index Coords::getNumVertices()
{
    return getSize();
}

Index Coords::getNumCoords()
{
    return getSize();
}

Index Coords::getNumCoords() const
{
    return getSize();
}

Normals::const_ptr Coords::normals() const
{
    return Normals::as(d()->normals.getObject());
}

void Coords::setNormals(Normals::const_ptr normals)
{
    d()->normals = normals;
}

Vector3 Coords::getVertex(Index v) const
{
    assert(v < getSize());
    return Vector3(x()[v], y()[v], z()[v]);
}

V_OBJECT_TYPE(Coords, Object::COORD)
V_OBJECT_CTOR(Coords)
V_OBJECT_IMPL(Coords)
V_OBJECT_INST(Coords)

} // namespace vistle
