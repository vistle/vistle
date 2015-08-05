#ifndef UNSTR_IMPL_H
#define UNSTR_IMPL_H

namespace vistle {

template<class Archive>
void UnstructuredGrid::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base_indexed", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("type_list", tl);
   arrayValid(tl);
}

} // namespace vistle

#endif
