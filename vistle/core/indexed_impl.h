#ifndef INDEXED_IMPL_H
#define INDEXED_IMPL_H

namespace vistle {
template<class Archive>
void Indexed::Data::serialize(Archive &ar, const unsigned int version)
{
   ar & V_NAME("base:coords", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("element_list", *el);
   ar & V_NAME("connection_list", *cl);
}

} // namespace vistle

#endif

