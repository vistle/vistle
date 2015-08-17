#include "coordswradius.h"

namespace vistle {

CoordsWithRadius::CoordsWithRadius(const Index numCoords,
         const Meta &meta)
   : CoordsWithRadius::Base(static_cast<Data *>(NULL))
{
    refreshImpl();
}

void CoordsWithRadius::refreshImpl() const {

    m_r = (d() && d()->r.valid()) ? d()->r->data() : nullptr;
}

void CoordsWithRadius::refresh() const {
    Base::refresh();
    refreshImpl();
}

bool CoordsWithRadius::isEmpty() const {

   return Base::isEmpty();
}

bool CoordsWithRadius::checkImpl() const {

   V_CHECK (d()->r->check());
   V_CHECK (getNumVertices() == d()->r->size());
   return true;
}

CoordsWithRadius::Data::Data(const Index numCoords,
             Type id,
             const std::string & name,
             const Meta &meta)
   : CoordsWithRadius::Base::Data(numCoords,
         id, name, meta)
, r(numCoords)
{
}

CoordsWithRadius::Data::Data(const CoordsWithRadius::Data &o, const std::string &n)
: CoordsWithRadius::Base::Data(o, n)
, r(o.r)
{
}

CoordsWithRadius::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id)
: CoordsWithRadius::Base::Data(o, n, id)
, r(o.x[0]->size())
{
}

CoordsWithRadius::Data *CoordsWithRadius::Data::create(const std::string &objId, Type id, const Index numCoords,
                      const Meta &meta) {

   assert("should never be called" == NULL);

   return NULL;
}

//V_OBJECT_TYPE(CoordsWithRadius, Object::COORDWRADIUS);
V_OBJECT_CTOR_REFRESH(CoordsWithRadius);

} // namespace vistle
