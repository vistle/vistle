#include "coordswradius.h"
#include "coordswradius_impl.h"
#include "archives.h"
#include "validate.h"

namespace vistle {

CoordsWithRadius::CoordsWithRadius(const size_t numCoords, const Meta &meta)
: CoordsWithRadius::Base(static_cast<Data *>(NULL))
{
    refreshImpl();
}

void CoordsWithRadius::refreshImpl() const
{
    const Data *d = static_cast<Data *>(m_data);
    if (d) {
        m_r = d->r;
    } else {
        m_r = nullptr;
    }
}

bool CoordsWithRadius::isEmpty()
{
    return Base::isEmpty();
}

bool CoordsWithRadius::isEmpty() const
{
    return Base::isEmpty();
}

bool CoordsWithRadius::checkImpl(std::ostream &os, bool quick) const
{
    VALIDATE_INDEX(d()->r->size());

    VALIDATE(d()->r->check(os));
    VALIDATE(getNumVertices() == d()->r->size());

    VALIDATE_SUB(normals());
    VALIDATE_SUBSIZE(normals(), getSize());

    if (quick)
        return true;

    VALIDATE_GEQ_P(d()->r, 0);

    return true;
}


void CoordsWithRadius::print(std::ostream &os, bool verbose) const
{
    Base::print(os);
    os << " r(";
    d()->r->print(os, verbose);
    os << ")";
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
