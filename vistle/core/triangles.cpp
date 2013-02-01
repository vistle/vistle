#include "triangles.h"

namespace vistle {

Triangles::Triangles(const size_t numCorners, const size_t numVertices,
                     const Meta &meta)
   : Triangles::Base(Triangles::Data::create(numCorners, numVertices,
            meta)) {
}

Triangles::Data::Data(const Triangles::Data &o, const std::string &n)
: Triangles::Base::Data(o, n)
, cl(o.cl)
{
}

Triangles::Data::Data(const size_t numCorners, const size_t numVertices,
                     const std::string & name,
                     const Meta &meta)
   : Base::Data(numCorners,
         Object::TRIANGLES, name,
         meta)
   , cl(new ShmVector<size_t>(numCorners))
{
}


Triangles::Data * Triangles::Data::create(const size_t numCorners,
                              const size_t numVertices,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *t = shm<Data>::construct(name)(numCorners, numVertices, name, meta);
   publish(t);

   return t;
}

size_t Triangles::getNumCorners() const {

   return cl().size();
}

size_t Triangles::getNumVertices() const {

   return x(0).size();
}

V_OBJECT_TYPE(Triangles, Object::TRIANGLES);

} // namespace vistle
