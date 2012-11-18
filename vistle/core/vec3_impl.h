#ifndef VEC3_IMPL_H
#define VEC3_IMPL_H

namespace vistle {

template <class T>
Vec3<T>::Vec3(const size_t size,
        const int block, const int timestep)
      : Object(Data::create(size, block, timestep)) {
   }

template <class T>
void Vec3<T>::setSize(const size_t size) {
      d()->x->resize(size);
      d()->y->resize(size);
      d()->z->resize(size);
   }

template <class T>
template <class Archive>
void Vec3<T>::Data::serialize(Archive &ar, const unsigned int version) {

   ar & V_NAME("base", boost::serialization::base_object<Base::Data>(*this));
   ar & V_NAME("x", *x);
   ar & V_NAME("y", *y);
   ar & V_NAME("z", *z);
}

} // namespace vistle

#endif
