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

    //FIXME
#if 0
   bool haveNormals = false;
   if (normals)
      normals->unref();
   normals = NULL;
   ar & V_NAME("haveNormals", haveNormals);
   if (haveNormals) {
      Normals *n = NULL;
      ar & V_NAME("normals", n);
      objectValid(n);
      assert(n);
      normals = n->d();
      n->ref();
   }
#endif
}

template<class Archive>
void Coords::Data::save(Archive &ar, const unsigned int version) const {

   ar & V_NAME("normals", normals);
}

} // namespace vistle

#endif
