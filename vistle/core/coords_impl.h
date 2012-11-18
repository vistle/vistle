#ifndef COORDS_IMPL_H
#define COORDS_IMPL_H

namespace vistle {

template<class Archive>
void Coords::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

namespace boost {
namespace serialization {

template<>
void access::destroy(const vistle::Object *t);

template<>
void access::construct(vistle::Object *t);

} // namespace serialization
} // namespace boost

#endif

