#ifndef GEOMETRY_IMPL_H
#define GEOMETRY_IMPL_H

namespace vistle {

template<class Archive>
void Geometry::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));

   boost::serialization::split_member(ar, *this, version); \
}

template<class Archive>
void Geometry::Data::load(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));

   bool haveGeometry = false;
   geometry = NULL;
   ar & V_NAME("haveGeometry", haveGeometry);
   if (haveGeometry) {
      Object *obj = NULL;
      ar & V_NAME("geometry", obj);
      assert(obj);
      geometry = obj->d();
      obj->ref();
   }

   bool haveColors = false;
   colors = NULL;
   ar & V_NAME("haveColors", haveColors);
   if (haveColors) {
      Object *obj = NULL;
      ar & V_NAME("colors", obj);
      assert(obj);
      colors = obj->d();
      obj->ref();
   }

   bool haveNormals = false;
   normals = NULL;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals) {
      Object *obj = NULL;
      ar & V_NAME("normals", obj);
      assert(obj);
      normals = obj->d();
      obj->ref();
   }

   bool haveTexture = false;
   texture = NULL;
   ar & V_NAME("haveTexture", haveTexture);
   if (haveTexture) {
      Object *obj = NULL;
      ar & V_NAME("texture", obj);
      assert(obj);
      texture = obj->d();
      obj->ref();
   }
}

template<class Archive>
void Geometry::Data::save(Archive &ar, const unsigned int version) const {
   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));

   // make sure that boost::serialization's object tracking is not tricked
   // into thinking that all these are the same object
   const Object *g = NULL, *c = NULL, *n = NULL, *t = NULL;

   bool haveGeometry = geometry;
   ar & V_NAME("haveGeometry", haveGeometry);
   if (haveGeometry) {
      Object::const_ptr ptr = Object::create(&*geometry);
      g = &*ptr;
      ar & V_NAME("geometry", g);
   }

   bool haveColors = colors;
   ar & V_NAME("haveColors", haveColors);
   if (haveColors) {
      Object::const_ptr ptr = Object::create(&*colors);
      c = &*ptr;
      ar & V_NAME("colors", c);
   }

   bool haveNormals = normals;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals) {
      Object::const_ptr ptr = Object::create(&*normals);
      n = &*ptr;
      ar & V_NAME("normals", n);
   }

   bool haveTexture = texture;
   ar & V_NAME("haveTexture", haveTexture);
   if (haveTexture) {
      Object::const_ptr ptr = Object::create(&*texture);
      t = &*ptr;
      ar & V_NAME("texture", t);
   }
}

} // namespace vistle

#endif
