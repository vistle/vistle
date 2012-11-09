#ifndef VEC3_IMPL_H
#define VEC3_IMPL_H

namespace vistle {

template <class T>
Vec3<T>::Vec3(const size_t size,
        const int block, const int timestep)
      : Object(Data::create(size, block, timestep)) {
   }

template <class T>
size_t Vec3<T>::getSize() const {
   return d()->x->size();
}

template <class T>
void Vec3<T>::setSize(const size_t size) {
      d()->x->resize(size);
      d()->y->resize(size);
      d()->z->resize(size);
   }

template <class T>
typename shm<T>::vector &Vec3<T>::x() const {
   return *(*d()->x)();
}

template <class T>
typename shm<T>::vector &Vec3<T>::y() const {
   return *(*d()->y)();
}

template <class T>
typename shm<T>::vector &Vec3<T>::z() const {
   return *(*d()->z)();
}

template <class T>
typename shm<T>::vector &Vec3<T>::x(int c) const {
   assert(c >= 0 && c<=2 && "Vec3 only has three components");
   switch(c) {
      case 0:
         return x();
      case 1:
         return y();
      case 2:
         return z();
   }
   return x();
}

} // namespace vistle

#endif
