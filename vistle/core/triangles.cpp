#include "triangles.h"

namespace vistle {

Triangles::Triangles(const size_t numCorners, const size_t numVertices,
                     const int block, const int timestep)
   : Triangles::Base(Triangles::Data::create(numCorners, numVertices,
            block, timestep)) {
}

Triangles::Data::Data(const size_t numCorners, const size_t numVertices,
                     const std::string & name,
                     const int block, const int timestep)
   : Base::Data(numCorners,
         Object::TRIANGLES, name,
         block, timestep)
   , cl(new ShmVector<size_t>(numCorners))
{
}


Triangles::Data * Triangles::Data::create(const size_t numCorners,
                              const size_t numVertices,
                              const int block, const int timestep) {

   const std::string name = Shm::instance().createObjectID();
   Data *t = shm<Data>::construct(name)(numCorners, numVertices, name, block, timestep);
   publish(t);

   return t;
}

Triangles::Info *Triangles::getInfo(Triangles::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numCorners = getNumCorners();
   info->numVertices = getNumVertices();

   return info;
}

size_t Triangles::getNumCorners() const {

   return d()->cl->size();
}

size_t Triangles::getNumVertices() const {

   return d()->x->size();
}

V_OBJECT_TYPE(Triangles, Object::TRIANGLES);

} // namespace vistle
