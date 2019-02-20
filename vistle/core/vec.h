#ifndef VEC_H
#define VEC_H
#include "index.h"
#include "dimensions.h"
#include "object.h"
#include "vector.h"
#include "database.h"
#include "shmvector.h"

namespace vistle {

template <typename T, int Dim=1>
class Vec: public DataBase {
   V_OBJECT(Vec);

   static const int MaxDim = MaxDimension;
   static_assert(Dim > 0, "only positive Dim allowed");
   static_assert(Dim <= MaxDim, "Dim too large");

 public:
   typedef DataBase Base;
   typedef typename shm<T>::array array;
   typedef T Scalar;
   typedef VistleVector<Scalar,Dim> Vector;

   Vec(const Index size,
        const Meta &meta=Meta());

   Index getSize() const override {
      return (Index)(d()->x[0]->size());
   }

   void resetArrays() override;
   void setSize(const Index size) override;
   void applyDimensionHint(Object::const_ptr grid) override;
   void setExact(bool exact) override;

   array &x(int c=0) { return *d()->x[c]; }
   array &y() { return *d()->x[1]; }
   array &z() { return *d()->x[2]; }
   array &w() { return *d()->x[3]; }

   const T *x(int c=0) const { return m_x[c]; }
   const T *y() const { return m_x[1]; }
   const T *z() const { return m_x[2]; }
   const T *w() const { return m_x[3]; }

   void updateInternals() override;
   std::pair<Vector, Vector> getMinMax() const;

 private:
   mutable const T *m_x[MaxDim];
   mutable Index m_size;

 public:
   struct Data: public Base::Data {

      Vector min;
      Vector max;
      ShmVector<T> x[Dim];
      // when used as Vec
      Data(const Index size = 0, const std::string &name = "",
            const Meta &meta=Meta());
      // when used as base of another data structure
      Data(const Index size, Type id, const std::string &name,
            const Meta &meta=Meta());
      Data(const Data &other, const std::string &name, Type id=UNKNOWN);
      Data(Object::Type id, const std::string &name, const Meta &meta);
      static Data *create(Index size=0, const Meta &meta=Meta());
      static Data *createNamed(Object::Type id, const std::string &name, const Meta &meta=Meta());

   protected:
      void setExact(bool exact);

   private:
      friend class Vec;
      ARCHIVE_ACCESS
      template<class Archive>
      void serialize(Archive &ar);
      void initData();
      void updateBounds();
      void invalidateBounds();
      bool boundsValid() const;
   };
};

} // namespace vistle
#endif

#ifdef VISTLE_IMPL
#include "vec_impl.h"
#endif
