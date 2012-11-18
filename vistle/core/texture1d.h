#ifndef TEXTURE1D_H
#define TEXTURE1D_H


#include "scalar.h"
#include "shm.h"
#include "object.h"

namespace vistle {

class Texture1D: public Object {
   V_OBJECT(Texture1D);

 public:
   typedef Object Base;

   Texture1D(const size_t width,
         const Scalar min, const Scalar max,
         const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getWidth() const;
   shm<unsigned char>::vector &pixels() const { return *(*d()->pixels)(); }
   shm<Scalar>::vector &coords() const { return *(*d()->coords)(); }

 protected:
   struct Data: public Base::Data {
      Scalar min;
      Scalar max;

      ShmVector<unsigned char>::ptr pixels;
      ShmVector<Scalar>::ptr coords;

      static Data *create(const size_t width = 0,
            const Scalar min = 0, const Scalar max = 0,
            const int block = -1, const int timestep = -1);
      Data(const std::string &name = "", const size_t size = 0,
            const Scalar min = 0, const Scalar max = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class Texture1D;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "texture1d_impl.h"
#endif
#endif
