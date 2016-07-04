#include "placeholder.h"
#include "placeholder_impl.h"
#include "archives.h"

namespace vistle {

PlaceHolder::PlaceHolder(const std::string &originalName, const Meta &originalMeta, Object::Type originalType)
   : PlaceHolder::Base(PlaceHolder::Data::create("", originalName, originalMeta, originalType))
{
    refreshImpl();
}

void PlaceHolder::refreshImpl() const {
}

bool PlaceHolder::isEmpty() const {

   return true;
}

bool PlaceHolder::checkImpl() const {

   return true;
}

PlaceHolder::Data::Data(const PlaceHolder::Data &o, const std::string &n)
: PlaceHolder::Base::Data(o, n)
, real(o.real)
, originalName(o.originalName)
, originalMeta(o.originalMeta)
, originalType(o.originalType)
{
}

PlaceHolder::Data::Data(const std::string & name,
      const std::string &originalName,
      const Meta &m,
      Object::Type originalType)
: PlaceHolder::Base::Data(Object::PLACEHOLDER, name, m)
, originalName(originalName)
, originalMeta(m)
, originalType(originalType)
{
}

PlaceHolder::Data::~Data() {

}

PlaceHolder::Data * PlaceHolder::Data::create(const std::string &objId, const std::string &originalName, const Meta &originalMeta, Object::Type originalType) {

   const std::string name = Shm::the().createObjectId(objId);
   Data *p = shm<Data>::construct(name)(name, originalName, originalMeta, originalType);
   publish(p);

   return p;
}

void PlaceHolder::setReal(Object::const_ptr r) {

   d()->real = r;
}

Object::const_ptr PlaceHolder::real() const {

   return d()->real.getObject();
}

std::string PlaceHolder::originalName() const {

   return d()->originalName.operator std::string();
}

Object::Type PlaceHolder::originalType() const {

   return d()->originalType;
}

const Meta &PlaceHolder::originalMeta() const {

   return d()->originalMeta;
}

V_OBJECT_TYPE(PlaceHolder, Object::PLACEHOLDER);
V_OBJECT_CTOR(PlaceHolder);

} // namespace vistle
