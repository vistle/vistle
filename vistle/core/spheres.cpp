#include "spheres.h"

namespace vistle {

Spheres::Spheres(const Index numSpheres,
         const Meta &meta)
   : Spheres::Base(Spheres::Data::create(numSpheres, meta))
{
}

bool Spheres::isEmpty() const {

   return Base::isEmpty();
}

bool Spheres::checkImpl() const {

   V_CHECK (getNumVertices() == r().size());
   return true;
}

Index Spheres::getNumSpheres() const {

   return getNumCoords();
}

Spheres::Data::Data(const Index numSpheres,
             const std::string & name,
             const Meta &meta)
   : Spheres::Base::Data(numSpheres,
         Object::SPHERES, name, meta)
, r(new ShmVector<Scalar>(numSpheres))
{
}

Spheres::Data::Data(const Spheres::Data &o, const std::string &n)
: Spheres::Base::Data(o, n)
, r(o.r)
{
}

Spheres::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n)
: Spheres::Base::Data(o, n, Object::SPHERES)
, r(new ShmVector<Scalar>(o.x[0]->size()))
{
}

Spheres::Data *Spheres::Data::create(const Index numSpheres,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(numSpheres, name, meta);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Spheres, Object::SPHERES);

} // namespace vistle
