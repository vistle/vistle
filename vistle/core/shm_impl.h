#ifndef SHM_IMPL_H
#define SHM_IMPL_H

#include "archives.h"

namespace vistle {

shm_name_t::shm_name_t(const std::string &s) {

   size_t len = s.size();
   assert(len < sizeof(name));
   if (len >= sizeof(name))
      len = sizeof(name)-1;
   strncpy(name, &s[0], len);
   name[len] = '\0';
}

shm_name_t::operator const char *() const {
   return name;
}

shm_name_t::operator char *() {
   return name;
}

shm_name_t::operator std::string () const {
   return name;
}

std::string operator+(const std::string &s, const shm_name_t &n) {
   return s + n.name;
}

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
ShmVector<T>::ShmVector(Index size)
: m_refcount(0)
{
   std::string n(Shm::the().createObjectID());
   size_t nsize = n.size();
   if (nsize >= sizeof(m_name)) {
      nsize = sizeof(m_name)-1;
   }
   n.copy(m_name, nsize);
   assert(n.size() < sizeof(m_name));
   m_x = Shm::the().shm().construct<typename shm<T>::vector>((const char *)m_name)(size, T(), Shm::the().allocator());
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
void ShmVector<T>::operator delete(void *p) {
   return Shm::the().shm().deallocate(p);
}

template<typename T>
void ShmVector<T>::resize(Index s) {
   m_x->resize(s);
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
