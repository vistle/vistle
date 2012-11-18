#ifndef LINES_IMPL_H
#define LINES_IMPL_H

namespace vistle {

template<class Archive>
void Lines::Data::serialize(Archive &ar, const unsigned int version) {
   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
}

} // namespace vistle

#endif

