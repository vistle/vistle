#ifndef SHM_IMPL_H
#define SHM_IMPL_H

#include <boost/type_traits.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/push_back.hpp>

#include "scalars.h"
#include "celltreenode.h"

//#include "archives.h"

namespace vistle {

typedef boost::mpl::push_back<Scalars, CelltreeNode<sizeof(Index), 1>>::type VectorTypes1;
typedef boost::mpl::push_back<VectorTypes1, CelltreeNode<sizeof(Index), 2>>::type VectorTypes2;
typedef boost::mpl::push_back<VectorTypes2, CelltreeNode<sizeof(Index), 3>>::type VectorTypes;

#if 0
template<typename T>
ShmVector<T>::ptr::ptr(ShmVector *p)
: m_p(p)
{
   if (m_p)
      m_p->ref();
}

template<typename T>
ShmVector<T>::ptr::ptr(const ptr &ptr)
: m_p(ptr.m_p)
{
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
int ShmVector<T>::typeId() {
   const size_t pos = boost::mpl::find<Scalars, T>::type::pos::value;
   BOOST_STATIC_ASSERT(pos < boost::mpl::size<VectorTypes>::value);
   return pos;
}

template<typename T>
ShmVector<T>::ShmVector(Index size, const std::string &name)
: shm<T>::array(size)
, m_refcount(0)
{
   m_type = typeId();

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
bool ShmVector<T>::check() const {

   return m_type == typeId();
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
#endif

template<class Archive>
void shm_name_t::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<class Archive>
void shm_name_t::save(Archive &ar, const unsigned int version) const {

   std::string n(name.data());
   std::cerr << "SHM_NAME_T save: '" << n << "'" << std::endl;
   ar & boost::serialization::make_nvp("shm_name_t", n);
}

template<class Archive>
void shm_name_t::load(Archive &ar, const unsigned int version) {

   std::string n;
   ar & boost::serialization::make_nvp("shm_name_t", n);
   auto end = n.find('\0');
   if (end != std::string::npos) {
      n = n.substr(0, end);
   }
   if (n.size() < name.size()) {
      std::copy(n.begin(), n.end(), name.data());
      name[n.size()] = '\0';
   } else {
      std::cerr << "shm_name_t: name too long: " << n << " (" << n.size() << " chars)" << std::endl;
      memset(name.data(), 0, name.size());
      assert(n.size() < name.size());
   }
   std::cerr << "SHM_NAME_T load: '" << name.data() << "'" << std::endl;
}

#if 0
template<typename T>
template<class Archive>
void ShmVector<T>::ptr::serialize(Archive &ar, const unsigned int version) {

   boost::serialization::split_member(ar, *this, version);
}

template<typename T>
template<class Archive>
void ShmVector<T>::ptr::load(Archive &ar, const unsigned int version) {

   shm_name_t shmname;
   ar & boost::serialization::make_nvp("shm_name", shmname);
   std::string name = shmname;
   if (name == "(nil)") {
       m_p = nullptr;
   } else {
       auto obj = ar.currentObject();
       auto handler = ar.objectCompletionHandler();
       m_p = ar.template getArray<T>(name, [this, name, obj, handler]() -> void {
           //m_p = static_cast<ShmVector<T> *>(Shm::the().getArrayFromName(name));
           m_p = const_cast<ShmVector<T> *>(Shm::the().getArrayFromName<T>(name));
           assert(m_p);
           assert(m_p->m_name == name);
           m_p->ref();
           if (obj) {
               obj->referenceResolved(handler);
           }
       });
       if (m_p) {
           m_p->ref();
       } else {
           auto obj = ar.currentObject();
           if (obj)
               obj->arrayValid(*this);
       }
   }
   //ar & boost::serialization::make_nvp("shm_ptr", *m_p);
}

template<typename T>
template<class Archive>
void ShmVector<T>::ptr::save(Archive &ar, const unsigned int version) const {

    if (m_p) {
        ar & boost::serialization::make_nvp("shm_name", m_p->m_name);
    } else {
        shm_name_t name("(nil)");
        ar & boost::serialization::make_nvp("shm_name", name);
    }
   //ar & boost::serialization::make_nvp("shm_ptr", *m_p);
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

template<typename T>
const ShmVector<T> *Shm::getArrayFromName(const std::string &name) const {

   // we have to use char here, otherwise boost-internal consistency checks fail
   void *mem = vistle::shm<T>::find(name);
   if (mem) {
      return static_cast<ShmVector<T> *>(mem);
   }

   std::cerr << "Shm::getArrayFromName: did not find " << name << std::endl;

   return nullptr;
}
#endif

} // namespace vistle

#endif
