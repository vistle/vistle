#ifndef TEXTURE_IMPL_H
#define TEXTURE_IMPL_H

namespace vistle {

template<class Archive>
void Texture1D::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base:vec", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("pixels", *pixels);
   ar & V_NAME("min", min);
   ar & V_NAME("max", max);
}

} // namespace vistle
#endif
