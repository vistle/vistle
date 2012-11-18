#ifndef TRIANGLES_H
#define TRIANGLES_H

#include "shm.h"
#include "coords.h"

namespace vistle {

class Triangles: public Coords {
   V_OBJECT(Triangles);

 public:
   typedef Coords Base;

   struct Info: public Base::Info {
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Triangles(const size_t numCorners, const size_t numVertices,
             const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *(*d()->cl)(); }

 protected:
   struct Data: public Base::Data {

      ShmVector<size_t>::ptr cl;

      Data(const size_t numCorners = 0, const size_t numVertices = 0,
            const std::string & name = "",
            const int block = -1, const int timestep = -1);
      static Data *create(const size_t numCorners = 0, const size_t numVertices = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class Triangles;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "triangles_impl.h"
#endif
#endif
