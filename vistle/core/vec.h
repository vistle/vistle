#ifndef VEC_H
#define VEC_H

#include "scalar.h"
#include "shm.h"
#include "object.h"

namespace vistle {

template <typename T, int Dim=1>
class Vec: public Object {
   V_OBJECT(Vec);

   BOOST_STATIC_ASSERT(Dim > 0);

 public:
   typedef Object Base;

   Vec(const size_t size,
        const int block = -1, const int timestep = -1);

   size_t getSize() const {
      return d()->x[0]->size();
   }

   void setSize(const size_t size);

   typename shm<T>::vector &x(int c=0) const { return *(*d()->x[c])(); }
   typename shm<T>::vector &y() const { return *(*d()->x[1])(); }
   typename shm<T>::vector &z() const { return *(*d()->x[2])(); }
   typename shm<T>::vector &w() const { return *(*d()->x[3])(); }

 protected:
   struct Data: public Base::Data {

      typename ShmVector<T>::ptr x[Dim];
      // when used as Vec
      Data(const size_t size = 0, const std::string &name = "",
            const int block = -1, const int timestep = -1)
         : Base::Data(s_type, name, block, timestep)
      {
         for (int c=0; c<Dim; ++c)
            x[c] = new ShmVector<T>(size);
      }
      // when used as base of another data structure
      Data(const size_t size, Type id, const std::string &name,
            const int block, const int timestep)
         : Base::Data(id, name, block, timestep)
      {
         for (int c=0; c<Dim; ++c)
            x[c] = new ShmVector<T>(size);
      }
      static Data *create(size_t size = 0, const int block = -1, const int timestep = -1) {
         std::string name = Shm::the().createObjectID();
         Data *t = shm<Data>::construct(name)(size, name, block, timestep);
         publish(t);

         return t;
      }

      private:
      friend class Vec;
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version);
   };

 private:
   static const Object::Type s_type;
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "vec_impl.h"
#endif
#endif
