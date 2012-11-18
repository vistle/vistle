#ifndef COORDS_H
#define COORDS_H

#include "scalar.h"
#include "shm.h"
#include "object.h"
#include "vec3.h"

namespace vistle {

class Coords: public Vec3<Scalar> {
   V_OBJECT(Coords);

 public:
   typedef Vec3<Scalar> Base;

   struct Info: public Base::Info {
      uint64_t numVertices;
   };

   Coords(const size_t numVertices,
             const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCoords() const;
   size_t getNumVertices() const;

 protected:
   struct Data: public Base::Data {

      Data(const size_t numVertices = 0,
            Type id = UNKNOWN, const std::string & name = "",
            const int block = -1, const int timestep = -1);
      static Data *create(Type id = UNKNOWN, const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class Coords;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "coords_impl.h"
#endif
#endif
