#include "placeholder.h"
#include "placeholder_impl.h"

namespace vistle {

PlaceHolder::PlaceHolder(const std::string &originalName, const Meta &originalMeta)
   : PlaceHolder::Base(PlaceHolder::Data::create(originalName, originalMeta))
{
}

bool PlaceHolder::checkImpl() const {

   return true;
}

PlaceHolder::Data::Data(const PlaceHolder::Data &o, const std::string &n)
: PlaceHolder::Base::Data(o, n)
, real(o.real)
{
}

PlaceHolder::Data::Data(const std::string & name,
      const std::string &originalName,
      const Meta &m)
   : PlaceHolder::Base::Data(Object::PLACEHOLDER, name, meta)
   , real(NULL)
{
   
   if (real)
      real->ref();
}

PlaceHolder::Data::~Data() {

   if (real)
      real->unref();
}

PlaceHolder::Data * PlaceHolder::Data::create() {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(name, "", Meta());
   publish(p);

   return p;
}

PlaceHolder::Data * PlaceHolder::Data::create(const std::string &originalName, const Meta &originalMeta) {

   const std::string name = Shm::the().createObjectID();
   Data *p = shm<Data>::construct(name)(name, originalName, originalMeta);
   publish(p);

   return p;
}

void PlaceHolder::setReal(Object::const_ptr r) {

   if (Object::const_ptr old = real())
      old->unref();
   d()->real = r->d();
   if (r)
      r->ref();
}

Object::const_ptr PlaceHolder::real() const {

   return Object::create(&*d()->real);
}

V_OBJECT_TYPE(PlaceHolder, Object::PLACEHOLDER);

} // namespace vistle
