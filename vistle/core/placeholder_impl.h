#ifndef PLACEHOLDER_IMPL_H
#define PLACEHOLDER_IMPL_H

namespace vistle {

template<class Archive>
void PlaceHolder::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base:object", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif
