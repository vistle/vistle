#ifndef COORDSWITHRADIUS_H
#define COORDSWITHRADIUS_H


#include "coords.h"

namespace vistle {

class  V_COREEXPORT CoordsWithRadius: public Coords {
   V_OBJECT(CoordsWithRadius);

   public:
   typedef Coords Base;

   CoordsWithRadius(const Index numCoords,
         const Meta &meta=Meta());
   void refresh() const override;

   shm<Scalar>::array &r() { return *(d()->r); }
   const Scalar *r() const { return m_r; }

   private:
   mutable const Scalar *m_r;
   void refreshImpl() const;

   V_DATA_BEGIN(CoordsWithRadius);
      ShmVector<Scalar> r;

      Data(const Index numCoords = 0,
            Type id = UNKNOWN, const std::string & name = "",
            const Meta &meta=Meta());
      Data(const Vec<Scalar, 3>::Data &o, const std::string &n, Type id);
      static Data *create(const std::string &name="", Type id = UNKNOWN, const Index numCoords = 0,
            const Meta &meta=Meta());

   V_DATA_END(CoordsWithRadius);
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(CoordsWithRadius)

} // namespace vistle

#ifdef VISTLE_IMPL
#include "coordswradius_impl.h"
#endif

#endif
