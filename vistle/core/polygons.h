#ifndef POLYGONS_H
#define POLYGONS_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"

namespace vistle {

class Polygons: public Indexed {
   V_OBJECT(Polygons);

 public:
   typedef Indexed Base;

   struct Info: public Base::Info {
   };

   Polygons(const size_t numElements= 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

 protected:
   struct Data: public Base::Data {

      Data(const size_t numElements, const size_t numCorners,
            const size_t numVertices, const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numElements= 0,
            const size_t numCorners = 0,
            const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

} // namespace vistle
#endif
