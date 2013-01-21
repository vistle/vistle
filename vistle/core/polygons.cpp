#include "polygons.h"

namespace vistle {

Polygons::Polygons(const size_t numElements,
      const size_t numCorners,
      const size_t numVertices,
      const int block, const int timestep)
: Polygons::Base(Polygons::Data::create(numElements, numCorners, numVertices, block, timestep))
{
}

Polygons::Data::Data(const Polygons::Data &o, const std::string &n)
: Polygons::Base::Data(o, n)
{
}

Polygons::Data::Data(const size_t numElements, const size_t numCorners,
                   const size_t numVertices, const std::string & name,
                   const int block, const int timestep)
   : Polygons::Base::Data(numElements, numCorners, numVertices,
         Object::POLYGONS, name, block, timestep)
{
}


Polygons::Data * Polygons::Data::create(const size_t numElements,
                            const size_t numCorners,
                            const size_t numVertices,
                            const int block, const int timestep) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, block, timestep);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Polygons, Object::POLYGONS);

} // namespace vistle
