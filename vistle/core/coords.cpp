#include "coords.h"

namespace vistle {

Coords::Coords(const size_t numVertices,
             const Meta &meta)
   : Coords::Base(numVertices, meta)
{}

bool Coords::checkImpl() const {

   return true;
}

Coords::Data::Data(const size_t numVertices,
      Type id, const std::string &name,
      const Meta &meta)
   : Coords::Base::Data(numVertices,
         id, name,
         meta)
{
}

Coords::Data::Data(const Coords::Data &o, const std::string &n)
: Coords::Base::Data(o, n)
{
}

Coords::Data *Coords::Data::create(Type id, const size_t numVertices,
            const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}

size_t Coords::getNumVertices() const {

   return getSize();
}

size_t Coords::getNumCoords() const {

   return getSize();
}

V_SERIALIZERS(Coords);

} // namespace vistle
