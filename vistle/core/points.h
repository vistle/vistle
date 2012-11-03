#ifndef POINTS_H
#define POINTS_H


#include "coords.h"

namespace vistle {

class Points: public Coords {
   V_OBJECT(Points);

   public:
   typedef Coords Base;

   size_t getNumPoints() const;

   protected:
   struct Data: public Base::Data {

      Data(const size_t numPoints,
            const std::string & name,
            const int block, const int timestep);
      static Data *create(const size_t numPoints,
            const int block, const int timestep);

      private:
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version) {
            ar & boost::serialization::base_object<Base::Data>(*this);
         }
   };

 private:
   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base>(*this);
      }
};

} // namespace vistle
#endif
