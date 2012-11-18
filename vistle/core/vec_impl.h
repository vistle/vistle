#ifndef VEC_IMPL_H
#define VEC_IMPL_H

namespace vistle {

template <class T>
Vec<T>::Vec(const size_t size,
      const int block, const int timestep)
   : Vec::Object(Data::create(size, block, timestep)) {
}

template <class T>
void Vec<T>::setSize(const size_t size) {
   d()->x->resize(size);
}

template <class T>
template <class Archive>
void Vec<T>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("x", *x);
}

} // namespace vistle

#endif
