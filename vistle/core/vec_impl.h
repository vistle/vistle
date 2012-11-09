#ifndef VEC_IMPL_H
#define VEC_IMPL_H

namespace vistle {

template <class T>
Vec<T>::Vec(const size_t size,
      const int block, const int timestep)
   : Vec::Object(Data::create(size, block, timestep)) {
}

template <class T>
size_t Vec<T>::getSize() const {
   return d()->x->size();
}

template <class T>
void Vec<T>::setSize(const size_t size) {
   d()->x->resize(size);
}

template <class T>
typename shm<T>::vector &Vec<T>::x() const { return *(*d()->x)(); }

template <class T>
typename shm<T>::vector &Vec<T>::x(int c) const { assert(c == 0 && "Vec only has one component"); return x(); }

} // namespace vistle

#endif
