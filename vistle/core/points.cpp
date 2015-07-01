#include "points.h"

namespace vistle {

Points::Points(const Index numPoints,
         const Meta &meta)
   : Points::Base(Points::Data::create("", numPoints, meta))
{
}

bool Points::isEmpty() const {

   return Base::isEmpty();
}

bool Points::checkImpl() const {

   return true;
}

Index Points::getNumPoints() const {

   return getNumCoords();
}

Points::Data::Data(const Index numPoints,
             const std::string & name,
             const Meta &meta)
   : Points::Base::Data(numPoints,
         Object::POINTS, name, meta)
{
}

Points::Data::Data(const Points::Data &o, const std::string &n)
: Points::Base::Data(o, n)
{
}

Points::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n)
: Points::Base::Data(o, n, Object::POINTS)
{
}

Points::Data *Points::Data::create(const std::string &objId, const Index numPoints,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *p = shm<Data>::construct(name)(numPoints, name, meta);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Points, Object::POINTS);

} // namespace vistle
