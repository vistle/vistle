#include "indexed.h"

namespace vistle {

Indexed::Indexed(const size_t numElements, const size_t numCorners,
                      const size_t numVertices,
                      const Meta &meta)
   : Indexed::Base(static_cast<Data *>(NULL))
{
}

Indexed::Data::Data(const size_t numElements, const size_t numCorners,
             const size_t numVertices,
             Type id, const std::string & name,
             const Meta &meta)
   : Indexed::Base::Data(numVertices, id, name,
         meta)
   , el(new ShmVector<size_t>(numElements))
   , cl(new ShmVector<size_t>(numCorners))
{
}

Indexed::Data::Data(const Indexed::Data &o, const std::string &name)
: Indexed::Base::Data(o, name)
, el(o.el)
, cl(o.cl)
{
}

Indexed::Data *Indexed::Data::create(Type id,
            const size_t numElements, const size_t numCorners,
            const size_t numVertices,
            const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}


size_t Indexed::getNumElements() const {

   return el().size();
}

size_t Indexed::getNumCorners() const {

   return cl().size();
}

size_t Indexed::getNumVertices() const {

   return x(0).size();
}

V_SERIALIZERS(Indexed);

} // namespace vistle
