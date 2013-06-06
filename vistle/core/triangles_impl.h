#ifndef TRIANGLES_IMPL_H
#define TRIANGLES_IMPL_H

namespace vistle {

template<class Archive>
void Triangles::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("connection_list", *cl);
}

} // namespace vistle
#endif

