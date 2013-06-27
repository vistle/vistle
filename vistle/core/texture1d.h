#ifndef TEXTURE1D_H
#define TEXTURE1D_H


#include "scalar.h"
#include "shm.h"
#include "object.h"

namespace vistle {

class V_COREEXPORT Texture1D: public Object {
   V_OBJECT(Texture1D);

 public:
   typedef Object Base;

   Texture1D(const Index width,
         const Scalar min, const Scalar max,
         const Meta &meta=Meta());

   Index getNumElements() const;
   Index getWidth() const;
   array<unsigned char> &pixels() const { return *(*d()->pixels)(); }
   array<Scalar> &coords() const { return *(*d()->coords)(); }

   V_DATA_BEGIN(Texture1D);
      Scalar min;
      Scalar max;

      ShmVector<unsigned char>::ptr pixels;
      ShmVector<Scalar>::ptr coords;

      static Data *create(const Index width = 0,
            const Scalar min = 0, const Scalar max = 0,
            const Meta &m=Meta());
      Data(const std::string &name = "", const Index size = 0,
            const Scalar min = 0, const Scalar max = 0,
            const Meta &m=Meta());

   V_DATA_END(Texture1D);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "texture1d_impl.h"
#endif
#endif
