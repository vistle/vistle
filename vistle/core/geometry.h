#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "shm.h"
#include "object.h"

namespace vistle {

class VCEXPORT Geometry: public Object {
   V_OBJECT(Geometry);

 public:
   typedef Object Base;

   Geometry(Object::const_ptr grid, const int block = -1, const int timestep = -1);

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

      Data(const std::string & name = "",
            const int block = -1, const int timestep = -1);
      ~Data();
      static Data *create(const int block = -1, const int timestep = -1);

      private:
      friend class Geometry;
      friend class boost::serialization::access;
      template<class Archive>
         void serialize(Archive &ar, const unsigned int version);
      template<class Archive>
         void load(Archive &ar, const unsigned int version);
      template<class Archive>
         void save(Archive &ar, const unsigned int version) const;
   };
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "geometry_impl.h"
#endif
#endif
