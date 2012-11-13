#ifndef POINTS_H
#define POINTS_H


#include "coords.h"

namespace vistle {

class Points: public Coords {
   V_OBJECT(Points);

   public:
   typedef Coords Base;

   Points(const size_t numPoints = 0,
         const int block = -1, const int timestep = -1);

   size_t getNumPoints() const;

   protected:
   struct Data: public Base::Data {

      Data(const size_t numPoints,
            const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numPoints,
            const int block, const int timestep);

      private:
      friend class Points;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
         }
   };
};

} // namespace vistle
#endif
