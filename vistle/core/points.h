#ifndef POINTS_H
#define POINTS_H


#include "coords.h"

namespace vistle {

class  VCEXPORT Points: public Coords {
   V_OBJECT(Points);

   public:
   typedef Coords Base;

   Points(const size_t numPoints,
         const int block = -1, const int timestep = -1);

   size_t getNumPoints() const;

   protected:
   struct Data: public Base::Data {

      Data(const size_t numPoints = 0,
            const std::string & name = "",
            const int block = -1, const int timestep = -1);
      static Data *create(const size_t numPoints = 0,
            const int block = -1, const int timestep = -1);

      private:
      friend class Points;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "points_impl.h"
#endif

#endif
