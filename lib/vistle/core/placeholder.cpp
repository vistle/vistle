#include "placeholder.h"
#include "placeholder_impl.h"
#include "archives.h"
#include "validate.h"
#include "shm_obj_ref_impl.h"

namespace vistle {

PlaceHolder::PlaceHolder(const std::string &originalName, const Meta &originalMeta, Object::Type originalType)
: PlaceHolder::Base(PlaceHolder::Data::create("", originalName, originalMeta, originalType))
{
    refreshImpl();
}

PlaceHolder::PlaceHolder(Object::const_ptr obj)
: PlaceHolder::Base(PlaceHolder::Data::create("", obj->getName(), obj->meta(), obj->getType()))
{
    copyAttributes(obj);
    setReal(obj);
    refreshImpl();
}

void PlaceHolder::setGeometry(Object::const_ptr geo)
{
    if (!geo) {
        d()->geometry = PlaceHolder::const_ptr();
        return;
    }

    if (auto ph = PlaceHolder::as(geo)) {
        d()->geometry = ph;
        return;
    }

    PlaceHolder::const_ptr ph(new PlaceHolder(geo));
    d()->geometry = ph;
}

PlaceHolder::const_ptr PlaceHolder::geometry() const
{
    return d()->geometry.getObject();
}

void PlaceHolder::setNormals(Object::const_ptr norm)
{
    if (!norm) {
        d()->normals = PlaceHolder::const_ptr();
        return;
    }

    if (auto ph = PlaceHolder::as(norm)) {
        d()->normals = ph;
        return;
    }

    PlaceHolder::const_ptr ph(new PlaceHolder(norm));
    d()->normals = ph;
}

PlaceHolder::const_ptr PlaceHolder::normals() const
{
    return d()->normals.getObject();
}

void PlaceHolder::setTexture(Object::const_ptr tex)
{
    if (!tex) {
        d()->texture = PlaceHolder::const_ptr();
        return;
    }

    if (auto ph = PlaceHolder::as(tex)) {
        d()->texture = ph;
    }

    PlaceHolder::const_ptr ph(new PlaceHolder(tex));
    d()->texture = ph;
}

PlaceHolder::const_ptr PlaceHolder::texture() const
{
    return d()->texture.getObject();
}

void PlaceHolder::refreshImpl() const
{}

bool PlaceHolder::isEmpty()
{
    return true;
}

bool PlaceHolder::isEmpty() const
{
    return true;
}

bool PlaceHolder::checkImpl(std::ostream &os, bool quick) const
{
    return true;

    VALIDATE_SUB(real());
    VALIDATE_SUB(geometry());
    VALIDATE_SUB(normals());
    VALIDATE_SUB(texture());
}

void PlaceHolder::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
    os << " orig(" << *d()->originalName << ")";
}

void PlaceHolder::Data::initData()
{
    originalType = Object::UNKNOWN;
}

PlaceHolder::Data::Data(const PlaceHolder::Data &o, const std::string &n)
: PlaceHolder::Base::Data(o, n)
, real(o.real)
, originalName(o.originalName)
, originalMeta(o.originalMeta)
, originalType(o.originalType)
{}

PlaceHolder::Data::Data(const std::string &name, const std::string &originalName, const Meta &m,
                        Object::Type originalType)
: PlaceHolder::Base::Data(Object::PLACEHOLDER, name, m)
, originalName(originalName)
, originalMeta(m)
, originalType(originalType)
{}

PlaceHolder::Data::~Data()
{}

PlaceHolder::Data *PlaceHolder::Data::create(const std::string &objId, const std::string &originalName,
                                             const Meta &originalMeta, Object::Type originalType)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *p = shm<Data>::construct(name)(name, originalName, originalMeta, originalType);
    publish(p);

    return p;
}

void PlaceHolder::setReal(Object::const_ptr r)
{
    d()->real = r;
}

Object::const_ptr PlaceHolder::real() const
{
    return d()->real.getObject();
}

std::string PlaceHolder::originalName() const
{
    return d()->originalName.operator std::string();
}

Object::Type PlaceHolder::originalType() const
{
    return d()->originalType;
}

const Meta &PlaceHolder::originalMeta() const
{
    return d()->originalMeta;
}

V_OBJECT_TYPE(PlaceHolder, Object::PLACEHOLDER)
V_OBJECT_CTOR(PlaceHolder)
V_OBJECT_IMPL(PlaceHolder)

} // namespace vistle
