#ifndef POLYGONS_IMPL_H
#define POLYGONS_IMPL_H

namespace vistle {

template<class Archive>
void Polygons::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:indexed", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif


