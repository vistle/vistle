#include "coords.h"

namespace boost {
namespace serialization {

#if 0
template<>
void access::destroy(const vistle::Coords *t)
{
   delete const_cast<vistle::Coords *>(t);
}

template<>
void access::construct(vistle::Coords *t)
{
   ::new(t) vistle::Coords(vistle::Shm::the().allocator());

}
#endif

} // namespace serialization
} // namespace boost

namespace vistle {

Coords::Coords(const size_t numVertices,
             const int block, const int timestep)
   : Coords::Base(numVertices, block, timestep)
{}

Coords::Data::Data(const size_t numVertices,
      Type id, const std::string &name,
      const int block, const int timestep)
   : Coords::Base::Data(numVertices,
         id, name,
         block, timestep)
{
}

Coords::Data *Coords::Data::create(Type id, const size_t numVertices,
            const int block, const int timestep) {

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
