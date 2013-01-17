#ifndef POINTS_H
#define POINTS_H


#include "coords.h"

namespace vistle {

class Points: public Coords {
   V_OBJECT(Points);

   public:
   typedef Coords Base;

   Points(const size_t numPoints,
         const int block = -1, const int timestep = -1);

   size_t getNumPoints() const;

   V_DATA_BEGIN(Points);

      Data(const size_t numPoints = 0,
            const std::string & name = "",
            const int block = -1, const int timestep = -1);
      static Data *create(const size_t numPoints = 0,
            const int block = -1, const int timestep = -1);

   V_DATA_END(Points);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "points_impl.h"
#endif

#endif
