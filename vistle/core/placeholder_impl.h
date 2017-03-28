#ifndef PLACEHOLDER_IMPL_H
#define PLACEHOLDER_IMPL_H

namespace vistle {

template<class Archive>
void PlaceHolder::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_object", boost::serialization::base_object<Base::Data>(*this));

   ar & V_NAME("orig_name", originalName);
   ar & V_NAME("orig_meta", originalMeta);
   ar & V_NAME("orig_type", originalType);
}

} // namespace vistle

#endif
