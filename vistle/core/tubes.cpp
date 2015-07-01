#include "tubes.h"

namespace vistle {

Tubes::Tubes(const Index numTubes, const Index numCoords,
         const Meta &meta)
   : Tubes::Base(Tubes::Data::create("", numTubes, numCoords, meta))
{
}

bool Tubes::isEmpty() const {

   return Base::isEmpty();
}

bool Tubes::checkImpl() const {

   V_CHECK (d()->style->size() == 3);
   V_CHECK (components().size() > 0);
   V_CHECK (components()[0] == 0);
   V_CHECK (getNumTubes() >= 0);
   V_CHECK (components()[getNumTubes()] <= getNumVertices());
   V_CHECK (getNumVertices() >= getNumTubes()*2);
   return true;
}

Index Tubes::getNumTubes() const {

   return components().size()-1;
}

Tubes::CapStyle Tubes::startStyle() const {

   return (CapStyle)(*d()->style)[0];
}

Tubes::CapStyle Tubes::jointStyle() const {

   return (CapStyle)(*d()->style)[1];
}

Tubes::CapStyle Tubes::endStyle() const {

   return (CapStyle)(*d()->style)[2];
}

void Tubes::setCapStyles(Tubes::CapStyle start, Tubes::CapStyle joint, Tubes::CapStyle end) {

   (*d()->style)[0] = start;
   (*d()->style)[1] = joint;
   (*d()->style)[2] = end;
}

Tubes::Data::Data(const Index numTubes,
             const Index numCoords,
             const std::string & name,
             const Meta &meta)
   : Tubes::Base::Data(numCoords,
         Object::TUBES, name, meta)
   , components(new ShmVector<Index>(numTubes+1))
   , style(new ShmVector<unsigned char>(3))
{
   (*components)[0] = 0;
   (*style)[0] = (*style)[1] = (*style)[2] = Tubes::Open;
}

Tubes::Data::Data(const Tubes::Data &o, const std::string &n)
: Tubes::Base::Data(o, n)
, components(o.components)
, style(o.style)
{
}

Tubes::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n)
: Tubes::Base::Data(o, n, Object::TUBES)
, components(new ShmVector<Index>(1))
, style(new ShmVector<unsigned char>(3))
{
   (*components)[0] = 0;
   (*style)[0] = (*style)[1] = (*style)[2] = Tubes::Open;
}

Tubes::Data *Tubes::Data::create(const std::string &objId, const Index numTubes, const Index numCoords,
                      const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *p = shm<Data>::construct(name)(numTubes, numCoords, name, meta);
   publish(p);

   return p;
}

V_OBJECT_TYPE(Tubes, Object::TUBES);

} // namespace vistle
