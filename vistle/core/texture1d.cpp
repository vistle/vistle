#include "texture1d.h"

namespace vistle {

Texture1D::Texture1D(const size_t width,
      const Scalar min, const Scalar max,
      const Meta &meta)
: Texture1D::Base(Texture1D::Data::create(width, min, max, meta))
{
}

Texture1D::Data::Data(const Texture1D::Data &o, const std::string &n)
: Texture1D::Base::Data(o, n)
, min(o.min)
, max(o.max)
, pixels(o.pixels)
, coords(o.coords)
{
}

Texture1D::Data::Data(const std::string &name, const size_t width,
                     const Scalar mi, const Scalar ma,
                     const Meta &meta)
   : Texture1D::Base::Data(Object::TEXTURE1D, name, meta)
   , min(mi)
   , max(ma)
   , pixels(new ShmVector<unsigned char>(width * 4))
   , coords(new ShmVector<Scalar>(1))
{
}

Texture1D::Data *Texture1D::Data::create(const size_t width,
                              const Scalar min, const Scalar max,
                              const Meta &meta) {

   const std::string name = Shm::the().createObjectID();
   Data *tex= shm<Data>::construct(name)(name, width, min, max, meta);
   publish(tex);

   return tex;
}

size_t Texture1D::getNumElements() const {

   return d()->coords->size();
}

size_t Texture1D::getWidth() const {

   return d()->pixels->size() / 4;
}

V_OBJECT_TYPE(Texture1D, Object::TEXTURE1D);

} // namespace vistle
