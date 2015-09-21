//#include "shm.h"

namespace vistle {

template<class T>
template<class Archive>
void shm_ref<T>::serialize(Archive &ar, const unsigned int version) {
   boost::serialization::split_member(ar, *this, version);
}

template<class T>
template<class Archive>
void shm_ref<T>::save(Archive &ar, const unsigned int version) const {

    ar & boost::serialization::make_nvp("shm_name", m_name);
}

template<class T>
template<class Archive>
void shm_ref<T>::load(Archive &ar, const unsigned int version) {
   ar & boost::serialization::make_nvp("shm_name", m_name);
   std::string name = m_name;

   unref();
   m_p = nullptr;
   auto obj = ar.currentObject();
   auto handler = ar.objectCompletionHandler();
   auto ref =  ar.template getArray<typename T::value_type>(name, [this, name, obj, handler]() -> void {
      //std::cerr << "array completion handler: " << name << std::endl;
      auto ref = Shm::the().getArrayFromName<typename T::value_type>(name);
      assert(ref);
      *this = ref;
      if (obj) {
         obj->referenceResolved(handler);
      }
   });
   if (ref) {
      *this = ref;
   } else {
      //std::cerr << "waiting for completion of " << name << std::endl;
      auto obj = ar.currentObject();
      if (obj)
         obj->arrayValid(*this);
   }
}

template<class T>
T &shm_ref<T>::operator*() {
    return *m_p;
}

template<class T>
const T &shm_ref<T>::operator*() const {
    return *m_p;
}

template<class T>
T *shm_ref<T>::operator->() {
    return m_p.get();
}

template<class T>
const T *shm_ref<T>::operator->() const {
    return m_p.get();
}

template<class T>
const shm_name_t &shm_ref<T>::name() const {
    return m_name;
}

}
