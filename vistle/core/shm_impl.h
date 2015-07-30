#ifndef SHM_IMPL_H
#define SHM_IMPL_H

#include <boost/type_traits.hpp>
//#include "archives.h"

namespace vistle {

template<typename T>
ShmVector<T>::ptr::ptr(ShmVector *p)
: m_p(p)
{
   if (m_p)
      m_p->ref();
}

template<typename T>
ShmVector<T>::ptr::ptr(const ptr &ptr) : m_p(ptr.m_p) {
   if (m_p)
      m_p->ref();
}

template<typename T>
ShmVector<T>::ptr::~ptr() {
   if (m_p)
      m_p->unref();
}

template<typename T>
typename ShmVector<T>::ptr &ShmVector<T>::ptr::operator=(const ptr &other) {
   if (m_p)
      m_p->unref();
   m_p = other.m_p;
   if (m_p)
      m_p->ref();
   return *this;
}

template<typename T>
typename ShmVector<T>::ptr &ShmVector<T>::ptr::operator=(ShmVector<T> *init) {
   if (m_p)
      m_p->unref();
   m_p = init;
   if (m_p)
      m_p->ref();
   return *this;
}

template<typename T>
ShmVector<T>::ShmVector(Index size, const std::string &name)
: m_refcount(0)
{
   std::string n(name.empty() ? Shm::the().createArrayId() : name);
   size_t nsize = n.size();
   if (nsize >= sizeof(m_name)) {
      nsize = sizeof(m_name)-1;
   }
   n.copy(m_name, nsize);
   assert(n.size() < sizeof(m_name));
#ifndef USE_BOOST_VECTOR
   if (boost::has_trivial_copy<T>::value) {
      m_x = Shm::the().shm().construct<typename shm<T>::array>((const char *)m_name)(size, Shm::the().allocator());
   } else
#endif
   {
      m_x = Shm::the().shm().construct<typename shm<T>::array>((const char *)m_name)(size, T(), Shm::the().allocator());
   }
#ifdef SHMDEBUG
   shm_handle_t handle = Shm::the().shm().get_handle_from_address(this);
   Shm::the().s_shmdebug->push_back(ShmDebugInfo('V', m_name, handle));
#endif
}

template<typename T>
int ShmVector<T>::refcount() const {

   return m_refcount;
}

template<typename T>
void* ShmVector<T>::operator new(size_t size) {
   return Shm::the().shm().allocate(size);
}

template<typename T>
void ShmVector<T>::operator delete(void *p, size_t size) {
    if (p)
        Shm::the().shm().deallocate(p);
}

template<typename T>
void ShmVector<T>::resize(Index s) {
   m_x->resize(s);
}

template<typename T>
void ShmVector<T>::clear() {
   m_x->clear();
}

template<typename T>
ShmVector<T>::~ShmVector() {

   shm<T>::destroy(m_name);
}

template<typename T>
void ShmVector<T>::ref() {
   boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(m_mutex);
   ++m_refcount;
}

template<typename T>
void ShmVector<T>::unref() {
   m_mutex.lock();
   --m_refcount;
   assert(m_refcount >= 0);
   if (m_refcount == 0) {
      m_mutex.unlock();
      delete this;
      return;
   }
   m_mutex.unlock();
}

template<class Archive>
void shm_name_t::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void shm_name_t::save(Archive &ar, const unsigned int version) const {

   std::string n(name.data());
   ar & boost::serialization::make_nvp("shm_name_t", n);
}

template<class Archive>
void shm_name_t::load(Archive &ar, const unsigned int version) {

   std::string n;
   ar & boost::serialization::make_nvp("shm_name_t", n);
   auto end = n.find('0');
   if (end != std::string::npos) {
      n = n.substr(0, end);
   }
   if (n.size() < name.size()) {
      std::copy(n.begin(), n.end(), name.data());
   } else {
      std::cerr << "shm_name_t: name too long: " << n << " (" << n.size() << " chars)" << std::endl;
      memset(name.data(), 0, name.size());
      assert(n.size() < name.size());
   }
}

template<typename T>
template<class Archive>
void ShmVector<T>::ptr::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<typename T>
template<class Archive>
void ShmVector<T>::ptr::load(Archive &ar, const unsigned int version) {

#if 0
   // get a unique identifier.  Using a constant means that all shared pointers
   // are held in the same set.  Thus we detect handle multiple pointers to the
   // same value instances in the archive.
   const void *shmvector_ptr_helper_id = 0;
   shared_ptr_serialization_helper &hlp = ar.template get_helper<shared_ptr_serialization_helper>(helper_instance_id);

   // load shared pointer object
   ...

   shared_ptr_serialization_helper &hlp = ar.template get_helper<shared_ptr_serialization_helper>(shared_ptr_helper_id);

   // look up object in helper object
   T *shared_object hlp.lookup(...);

   // if found, return the one from the table

   // load the shared_ptr data
   shared_ptr sp = ...

   // and add it to the table
   hlp.insert(sp);
   // implement shared_ptr_serialization_helper load algorithm with the aid of hlp

#endif
   ar & boost::serialization::make_nvp("shm_name", m_p->m_name);
   ar & boost::serialization::make_nvp("shm_ptr", *m_p);
}

template<typename T>
template<class Archive>
void ShmVector<T>::ptr::save(Archive &ar, const unsigned int version) const {

   ar & boost::serialization::make_nvp("shm_name", m_p->m_name);
   ar & boost::serialization::make_nvp("shm_ptr", *m_p);
}

template<typename T>
template<class Archive>
void ShmVector<T>::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<typename T>
template<class Archive>
void ShmVector<T>::load(Archive &ar, const unsigned int version) {

   Index s = 0;
   ar & boost::serialization::make_nvp("size", s);
   m_x->resize(s);
   ar & boost::serialization::make_nvp("elements", boost::serialization::make_array(&(*m_x)[0], m_x->size()));
}

template<typename T>
template<class Archive>
void ShmVector<T>::save(Archive &ar, const unsigned int version) const {

   Index s = m_x->size();
   ar & boost::serialization::make_nvp("size", s);
   ar & boost::serialization::make_nvp("elements", boost::serialization::make_array(&(*m_x)[0], m_x->size()));
}

} // namespace vistle

#endif
