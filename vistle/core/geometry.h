#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "shm.h"
#include "object.h"
#include "export.h"

namespace vistle {

class V_COREEXPORT Geometry: public Object {
   V_OBJECT(Geometry);

 public:
   typedef Object Base;

   Geometry(Object::const_ptr grid, const Meta &m=Meta());

   void setGeometry(Object::const_ptr g);
   void setColors(Object::const_ptr c);
   void setTexture(Object::const_ptr t);
   void setNormals(Object::const_ptr n);
   Object::const_ptr geometry() const;
   Object::const_ptr colors() const;
   Object::const_ptr normals() const;
   Object::const_ptr texture() const;

   V_DATA_BEGIN(Geometry);
      boost::interprocess::offset_ptr<Object::Data> geometry;
      boost::interprocess::offset_ptr<Object::Data> colors;
      boost::interprocess::offset_ptr<Object::Data> normals;
      boost::interprocess::offset_ptr<Object::Data> texture;

      Data(const std::string & name = "",
            const Meta &m=Meta());
      ~Data();
      static Data *create(const Meta &m=Meta());

      private:
      template<class Archive>
         void load(Archive &ar, const unsigned int version);
      template<class Archive>
         void save(Archive &ar, const unsigned int version) const;
   V_DATA_END(Geometry);
};

} // namespace vistle

#ifdef VISTLE_IMPL
#include "geometry_impl.h"
#endif
#endif
