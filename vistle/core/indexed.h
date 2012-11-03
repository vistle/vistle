#ifndef INDEXED_H
#define INDEXED_H


#include "scalar.h"
#include "shm.h"
#include "coords.h"

namespace vistle {

class Indexed: public Coords {
   V_OBJECT(Indexed);

 public:
   typedef Coords Base;

   struct Info: public Base::Info {
      uint64_t numElements;
      uint64_t numCorners;
      uint64_t numVertices;
   };

   Info *getInfo(Info *info = NULL) const;
   size_t getNumElements() const;
   size_t getNumCorners() const;
   size_t getNumVertices() const;

   shm<size_t>::vector &cl() const { return *(*d()->cl)(); }
   shm<size_t>::vector &el() const { return *(*d()->el)(); }

 protected:

   struct Data: public Base::Data {
      ShmVector<size_t>::ptr el, cl;

      Data(const size_t numElements, const size_t numCorners,
           const size_t numVertices,
            Type id, const std::string &name,
            int b = -1, int t = -1);
   };
};

} // namespace vistle
#endif
