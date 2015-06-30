#ifndef COORDS_IMPL_H
#define COORDS_IMPL_H

namespace vistle {

template<class Archive>
void Coords::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_vec", boost::serialization::base_object<Base::Data>(*this));

   boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void Coords::Data::load(Archive &ar, const unsigned int version) {

   bool haveNormals = false;
   if (normals)
      normals->unref();
   normals = NULL;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals) {
      Normals *n = NULL;
      ar & V_NAME("normals", n);
      assert(n);
      normals = n->d();
      n->ref();
   }
}

template<class Archive>
void Coords::Data::save(Archive &ar, const unsigned int version) const {

   // make sure that boost::serialization's object tracking is not tricked
   // into thinking that all these are the same object
   const Normals *n = nullptr;

   bool haveNormals = normals;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals) {
      Normals::const_ptr ptr = Normals::as(Object::create(&*normals));
      n = &*ptr;
      ar & V_NAME("normals", n);
   }
}

} // namespace vistle

#endif
