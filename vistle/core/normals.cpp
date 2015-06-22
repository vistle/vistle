#include "normals.h"

namespace vistle {

Normals::Normals(const Index numNormals,
         const Meta &meta)
   : Normals::Base(Normals::Data::create(numNormals, meta))
{
}

bool Normals::isEmpty() const {

   return Base::isEmpty();
}

bool Normals::checkImpl() const {

   return true;
}

Index Normals::getNumNormals() const {

   return getSize();
}

Normals::Data::Data(const Index numNormals,
             const std::string & name,
             const Meta &meta)
   : Normals::Base::Data(numNormals,
         Object::NORMALS, name, meta)
{
}

Normals::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n)
: Normals::Base::Data(o, n, Object::NORMALS)
{
}

Normals::Data::Data(const Normals::Data &o, const std::string &n)
: Normals::Base::Data(o, n)
{
}

Normals::Data *Normals::Data::create(const Index numNormals,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(numNormals, name, meta);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Normals, Object::NORMALS);

} // namespace vistle
