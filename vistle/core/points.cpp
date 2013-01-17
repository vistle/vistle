#include "points.h"

namespace vistle {

Points::Points(const size_t numPoints,
         const int block, const int timestep)
   : Points::Base(Points::Data::create(numPoints, block, timestep))
{
}

size_t Points::getNumPoints() const {

   return getNumCoords();
}

Points::Data::Data(const size_t numPoints,
             const std::string & name,
             const int block, const int timestep)
   : Points::Base::Data(numPoints,
         Object::POINTS, name, block, timestep)
{
}

Points::Data::Data(const Points::Data &o, const std::string &n)
: Points::Base::Data(o, n)
{
}

Points::Data *Points::Data::create(const size_t numPoints,
                      const int block, const int timestep) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(numPoints, name, block, timestep);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Points, Object::POINTS);

} // namespace vistle
