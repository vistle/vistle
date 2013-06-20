#include "indexed.h"

namespace vistle {

Indexed::Indexed(const Index numElements, const Index numCorners,
                      const Index numVertices,
                      const Meta &meta)
   : Indexed::Base(static_cast<Data *>(NULL))
{
}

bool Indexed::checkImpl() const {

   if (getNumElements() > 0) {
      V_CHECK (el()[0] < getNumCorners());
      V_CHECK (el()[getNumElements()-1] < getNumCorners());
   }

   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
   }

   return true;
}

Indexed::Data::Data(const Index numElements, const Index numCorners,
             const Index numVertices,
             Type id, const std::string & name,
             const Meta &meta)
   : Indexed::Base::Data(numVertices, id, name,
         meta)
   , el(new ShmVector<Index>(numElements))
   , cl(new ShmVector<Index>(numCorners))
{
}

Indexed::Data::Data(const Indexed::Data &o, const std::string &name)
: Indexed::Base::Data(o, name)
, el(o.el)
, cl(o.cl)
{
}

Indexed::Data *Indexed::Data::create(Type id,
            const Index numElements, const Index numCorners,
            const Index numVertices,
            const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}


Index Indexed::getNumElements() const {

   return el().size();
}

Index Indexed::getNumCorners() const {

   return cl().size();
}

Index Indexed::getNumVertices() const {

   return x(0).size();
}

V_SERIALIZERS(Indexed);

} // namespace vistle
