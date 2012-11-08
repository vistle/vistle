#ifndef LINES_H
#define LINES_H

#include "scalar.h"
#include "shm.h"
#include "indexed.h"

namespace vistle {

class Lines: public Indexed {
   V_OBJECT(Lines);

 public:
   typedef Indexed Base;

   struct Info: public Base::Info {
   };

   Info *getInfo(Info *info = NULL) const;

   Lines(const size_t numElements = 0, const size_t numCorners = 0,
         const size_t numVertices = 0,
         const int block = -1, const int timestep = -1);

 protected:
   struct Data: public Base::Data {

      Data(const size_t numElements, const size_t numCorners,
         const size_t numVertices, const std::string & name,
         const int block, const int timestep);
      static Data *create(const size_t numElements, const size_t numCorners,
         const size_t numVertices,
         const int block, const int timestep);

      private:
      friend class Lines;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
         }
   };
};

} // namespace vistle
#endif
