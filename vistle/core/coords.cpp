#include "coords.h"

namespace vistle {

Coords::Coords(const Index numVertices,
             const Meta &meta)
   : Coords::Base(static_cast<Data *>(NULL))
{}

bool Coords::isEmpty() const {

   return Base::isEmpty();
}

bool Coords::checkImpl() const {

   V_CHECK(!normals() || normals()->getNumNormals() == getSize());
   return true;
}

Coords::Data::Data(const Index numVertices,
      Type id, const std::string &name,
      const Meta &meta)
   : Coords::Base::Data(numVertices,
         id, name,
         meta)
{
}

Coords::Data::Data(const Coords::Data &o, const std::string &n)
: Coords::Base::Data(o, n)
, normals(o.normals)
{
}

Coords::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id)
: Coords::Base::Data(o, n, id)
{
}

Coords::Data *Coords::Data::create(const std::string &objId, Type id, const Index numVertices,
            const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}

Coords::Data::~Data() {
}

Index Coords::getNumVertices() const {

   return getSize();
}

Index Coords::getNumCoords() const {

   return getSize();
}

Normals::const_ptr Coords::normals() const {
   return Normals::as(d()->normals.getObject());
}

void Coords::setNormals(Normals::const_ptr normals) {

    d()->normals = normals;
}

//V_OBJECT_TYPE(Coords, Object::COORD);
V_OBJECT_CTOR(Coords);

} // namespace vistle
