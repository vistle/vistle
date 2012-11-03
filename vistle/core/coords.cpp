#include "coords.h"

namespace vistle {

Coords::Data::Data(const size_t numVertices,
      Type id, const std::string &name,
      const int block, const int timestep)
   : Coords::Base::Data(numVertices,
         id, name,
         block, timestep)
{
}

Coords::Info *Coords::getInfo(Coords::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numVertices = getNumVertices();

   return info;
}

size_t Coords::getNumVertices() const {

   return getSize();
}

} // namespace vistle
