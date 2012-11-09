#include "indexed.h"

namespace vistle {

Indexed::Info *Indexed::getInfo(Indexed::Info *info) const {

   if (!info)
      info = new Info;

   Base::getInfo(info);

   info->infosize = sizeof(Info);
   info->itemsize = 0;
   info->offset = 0;
   info->numVertices = getNumVertices();
   info->numElements = getNumElements();
   info->numCorners = getNumCorners();

   return info;
}

Indexed::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices,
             Type id, const std::string & name,
             const int block, const int timestep)
   : Indexed::Base::Data(numVertices, id, name,
         block, timestep)
   , el(new ShmVector<size_t>(numElements))
   , cl(new ShmVector<size_t>(numCorners))
{
}

size_t Indexed::getNumElements() const {

   return d()->el->size();
}

size_t Indexed::getNumCorners() const {

   return d()->cl->size();
}

size_t Indexed::getNumVertices() const {

   return d()->x->size();
}

V_SERIALIZERS(Indexed);

} // namespace vistle
