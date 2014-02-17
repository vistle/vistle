#ifndef SPHERES_H
#define SPHERES_H


#include "coords.h"

namespace vistle {

class  V_COREEXPORT Spheres: public Coords {
   V_OBJECT(Spheres);

   public:
   typedef Coords Base;

   Spheres(const Index numSpheres,
         const Meta &meta=Meta());

   Index getNumSpheres() const;

   shm<Scalar>::array &r() const { return *(*d()->r)(); }
   
   V_DATA_BEGIN(Spheres);
      ShmVector<Scalar>::ptr r;

      Data(const Index numSpheres = 0,
            const std::string & name = "",
            const Meta &meta=Meta());
      Data(const Vec<Scalar, 3>::Data &o, const std::string &n);
      static Data *create(const Index numSpheres = 0,
            const Meta &meta=Meta());

   V_DATA_END(Spheres);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "spheres_impl.h"
#endif

#endif
