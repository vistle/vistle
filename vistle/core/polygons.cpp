#include "polygons.h"

namespace vistle {

Polygons::Polygons(const Index numElements,
      const Index numCorners,
      const Index numVertices,
      const Meta &meta)
: Polygons::Base(Polygons::Data::create("", numElements, numCorners, numVertices, meta))
{
}

bool Polygons::isEmpty() const {

   return Base::isEmpty();
}

bool Polygons::checkImpl() const {

   return true;
}

Polygons::Data::Data(const Polygons::Data &o, const std::string &n)
: Polygons::Base::Data(o, n)
{
}

Polygons::Data::Data(const Index numElements, const Index numCorners,
                   const Index numVertices, const std::string & name,
                   const Meta &meta)
   : Polygons::Base::Data(numElements, numCorners, numVertices,
         Object::POLYGONS, name, meta)
{
}


Polygons::Data * Polygons::Data::create(const std::string &objId, const Index numElements,
                            const Index numCorners,
                            const Index numVertices,
                            const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *p = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Polygons, Object::POLYGONS);
V_OBJECT_CTOR(Polygons);

} // namespace vistle
