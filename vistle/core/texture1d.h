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

   Texture1D(const size_t width = 0,
         const Scalar min = 0, const Scalar max = 0,
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
      Data(const std::string &name, const size_t size,
            const Scalar min, const Scalar max,
            const int block, const int timestep);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & pixels;
         ar & coords;
         ar & min;
         ar & max;
      }
   };
};

} // namespace vistle
#endif
