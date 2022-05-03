#include "coordswradius.h"
#include "coordswradius_impl.h"
#include "archives.h"

namespace vistle {

CoordsWithRadius::CoordsWithRadius(const size_t numCoords, const Meta &meta)
: CoordsWithRadius::Base(static_cast<Data *>(NULL))
{
    refreshImpl();
}

void CoordsWithRadius::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    m_r = (d && d->r.valid()) ? d->r->data() : nullptr;
}

bool CoordsWithRadius::isEmpty()
{
    return Base::isEmpty();
}

bool CoordsWithRadius::isEmpty() const
{
    return Base::isEmpty();
}

bool CoordsWithRadius::checkImpl() const
{
    CHECK_OVERFLOW(d()->r->size());

    V_CHECK(d()->r->check());
    V_CHECK(getNumVertices() == d()->r->size());
    return true;
}

void CoordsWithRadius::resetArrays()
{
    Base::resetArrays();
    d()->r = ShmVector<Scalar>();
    d()->r.construct();
}

void CoordsWithRadius::setSize(const size_t size)
{
    Base::setSize(size);
    d()->r->resize(size);
}


void CoordsWithRadius::Data::initData()
{}

CoordsWithRadius::Data::Data(const size_t numCoords, Type id, const std::string &name, const Meta &meta)
: CoordsWithRadius::Base::Data(numCoords, id, name, meta)
{
    initData();
    r.construct(numCoords);
}

CoordsWithRadius::Data::Data(const CoordsWithRadius::Data &o, const std::string &n)
: CoordsWithRadius::Base::Data(o, n), r(o.r)
{
    initData();
}

CoordsWithRadius::Data::Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id)
: CoordsWithRadius::Base::Data(o, n, id)
{
    initData();
    r.construct(o.x[0]->size());
}

CoordsWithRadius::Data *CoordsWithRadius::Data::create(const std::string &objId, Type id, const size_t numCoords,
                                                       const Meta &meta)
{
    assert("should never be called" == NULL);

    return NULL;
}

V_OBJECT_TYPE(CoordsWithRadius, Object::COORDWRADIUS)
V_OBJECT_CTOR(CoordsWithRadius)
V_OBJECT_IMPL(CoordsWithRadius)
V_OBJECT_INST(CoordsWithRadius)

} // namespace vistle
