#ifndef TUBES_IMPL_H
#define TUBES_IMPL_H

namespace vistle {

template<class Archive>
void Tubes::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:coords_with_radius", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif
