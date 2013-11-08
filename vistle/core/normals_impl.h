#ifndef NORMALS_IMPL_H
#define NORMALS_IMPL_H

namespace vistle {

template<class Archive>
void Normals::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:coords", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif
