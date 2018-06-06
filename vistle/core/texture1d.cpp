#include "texture1d.h"

namespace vistle {

Texture1D::Texture1D(const Index width,
      const Scalar min, const Scalar max,
      const Meta &meta)
: Texture1D::Base(Texture1D::Data::create(width, min, max, meta))
{
    refreshImpl();
}

void Texture1D::refreshImpl() const {
}

bool Texture1D::isEmpty() const {
   return getWidth() == 0;
}

bool Texture1D::checkImpl() const {

    V_CHECK(d()->pixels);
    //V_CHECK (d()->min <= d()->max);
    return true;
}

void Texture1D::Data::initData() {
   min = max = 0.f;
}

Texture1D::Data::Data(const Texture1D::Data &o, const std::string &n)
: Texture1D::Base::Data(o, n)
, min(o.min)
, max(o.max)
, pixels(o.pixels)
{
   initData();
   min = o.min;
   max = o.max;
}

Texture1D::Data::Data(const std::string &name, const Index width,
                     const Scalar mi, const Scalar ma,
                     const Meta &meta)
   : Texture1D::Base::Data(0, Object::TEXTURE1D, name, meta)
   , min(mi)
   , max(ma)
{
   initData();
   min = mi;
   max = ma;
   pixels.construct(width * 4);
}

Texture1D::Data *Texture1D::Data::create(const Index width,
                              const Scalar min, const Scalar max,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectId();
   Data *tex= shm<Data>::construct(name)(name, width, min, max, meta);
   publish(tex);

   return tex;
}

Index Texture1D::getWidth() const {

   return d()->pixels->size() / 4;
}

V_OBJECT_TYPE(Texture1D, Object::TEXTURE1D);
V_OBJECT_CTOR(Texture1D);

} // namespace vistle
