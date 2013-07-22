#include "triangles.h"

namespace vistle {

Triangles::Triangles(const Index numCorners, const Index numVertices,
                     const Meta &meta)
   : Triangles::Base(Triangles::Data::create(numCorners, numVertices,
            meta)) {
}

bool Triangles::isEmpty() const {

   return getNumCorners()==0;
}

bool Triangles::checkImpl() const {

   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
   }
   return true;
}

Triangles::Data::Data(const Triangles::Data &o, const std::string &n)
: Triangles::Base::Data(o, n)
, cl(o.cl)
{
}

Triangles::Data::Data(const Index numCorners, const Index numVertices,
                     const std::string & name,
                     const Meta &meta)
   : Base::Data(numVertices,
         Object::TRIANGLES, name,
         meta)
   , cl(new ShmVector<Index>(numCorners))
{
}


Triangles::Data * Triangles::Data::create(const Index numCorners,
                              const Index numVertices,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *t = shm<Data>::construct(name)(numCorners, numVertices, name, meta);
   publish(t);

   return t;
}

Index Triangles::getNumCorners() const {

   return cl().size();
}

Index Triangles::getNumVertices() const {

   return x(0).size();
}

V_OBJECT_TYPE(Triangles, Object::TRIANGLES);

} // namespace vistle
