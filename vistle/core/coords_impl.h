#ifndef COORDS_IMPL_H
#define COORDS_IMPL_H

namespace vistle {

template<class Archive>
void Coords::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_vec", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("normals", normals);
}

} // namespace vistle

#endif
