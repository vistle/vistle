#include "triangles.h"
#include "archives.h"

namespace vistle {

Triangles::Triangles(const Index numCorners, const Index numCoords,
                     const Meta &meta)
   : Triangles::Base(Triangles::Data::create("", numCorners, numCoords,
            meta)) {
    refreshImpl();
}

void Triangles::refresh() const {
    Base::refresh();
    refreshImpl();
}

void Triangles::refreshImpl() const {
    m_cl = (d() && d()->cl.valid()) ? d()->cl->data() : nullptr;
}

bool Triangles::isEmpty() const {

   return getNumCoords()==0;
}

bool Triangles::checkImpl() const {

   V_CHECK (d()->cl->check());
   if (getNumCorners() > 0) {
      V_CHECK (cl()[0] < getNumVertices());
      V_CHECK (cl()[getNumCorners()-1] < getNumVertices());
      V_CHECK (getNumCorners() % 3 == 0);
   } else {
      V_CHECK (getNumCoords() % 3 == 0);
   }
   return true;
}

Triangles::Data::Data(const Triangles::Data &o, const std::string &n)
: Triangles::Base::Data(o, n)
, cl(o.cl)
{
}

Triangles::Data::Data(const Index numCorners, const Index numCoords,
                     const std::string & name,
                     const Meta &meta)
   : Base::Data(numCoords,
         Object::TRIANGLES, name,
         meta)
{
   cl.construct(numCorners);
}


Triangles::Data * Triangles::Data::create(const std::string &objId, const Index numCorners,
                              const Index numCoords,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *t = shm<Data>::construct(name)(numCorners, numCoords, name, meta);
   publish(t);

   return t;
}

Index Triangles::getNumElements() const {

   return getNumCorners()/3;
}

Index Triangles::getNumCorners() const {

   return d()->cl->size();
}

V_OBJECT_TYPE(Triangles, Object::TRIANGLES);
V_OBJECT_CTOR_REFRESH(Triangles);

} // namespace vistle
