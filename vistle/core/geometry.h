#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "shm.h"
#include "object.h"

namespace vistle {

class Geometry: public Object {
   V_OBJECT(Geometry);

 public:
   typedef Object Base;

   Geometry(const int block = -1, const int timestep = -1);

   Info *getInfo(Info *info = NULL) const;

   void setGeometry(Object::const_ptr g);
   void setColors(Object::const_ptr c);
   void setTexture(Object::const_ptr t);
   void setNormals(Object::const_ptr n);
   Object::const_ptr geometry() const;
   Object::const_ptr colors() const;
   Object::const_ptr normals() const;
   Object::const_ptr texture() const;

 protected:
   struct Data: public Base::Data {

      boost::interprocess::offset_ptr<Object::Data> geometry;
      boost::interprocess::offset_ptr<Object::Data> colors;
      boost::interprocess::offset_ptr<Object::Data> normals;
      boost::interprocess::offset_ptr<Object::Data> texture;

      Data(const std::string & name,
            const int block, const int timestep);
      ~Data();
      static Data *create(const int block = -1, const int timestep = -1);

      private:
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, const unsigned int version) {
         ar & boost::serialization::base_object<Base::Data>(*this);
         ar & geometry;
         ar & colors;
         ar & normals;
         ar & texture;
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
