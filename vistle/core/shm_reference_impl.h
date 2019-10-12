#ifndef SHM_REFERENCE_IMPL_H
#define SHM_REFERENCE_IMPL_H

#include "archives_config.h"
#include "object.h"

namespace vistle {

#if 0
template<class T>
shm_ref<T>::shm_ref()
    : m_name()
    , m_d(nullptr)
{
    ref();
}

template<class T>
shm_ref<T>::shm_ref(const std::string &name, shm_ref<T>::ShmType *p)
    : m_name(name)
    , m_d(p->d())
{
    ref();
}

template<class T>
shm_ref<T>::shm_ref(const shm_ref<T> &other)
    : m_name(other.m_name)
    , m_d(other.m_d)
{
    ref();
}

template<class T>
shm_ref<T>::shm_ref(const shm_name_t name)
    : m_name(name)
    , m_d(shm<T>::find(name))
{
    ref();
}

template<class T>
shm_ref<T>::~shm_ref() {
    unref();
}

template<class T>
template<typename... Args>
shm_ref<T> shm_ref<T>::create(const Args&... args) {
    shm_ref<T> result;
    result.construct(args...);
    return result;
}

template<class T>
bool shm_ref<T>::find() {
    auto mem = vistle::shm<ObjData>::find(m_name);
    m_d = mem;
    ref();

    return valid();
}

template<class T>
template<typename... Args>
void shm_ref<T>::construct(const Args&... args)
{
    unref();
    if (m_name.empty())
        m_name = Shm::the().createObjectId();
    m_d = shm<T>::construct(m_name)(args..., Shm::the().allocator());
    ref();
}

template<class T>
const shm_ref<T> &shm_ref<T>::operator=(const shm_ref<T> &rhs) {
    unref();
    m_name = rhs.m_name;
    m_d = rhs.m_d;
    ref();
    return *this;
}

template<class T>
const shm_ref<T> &shm_ref<T>::operator=(typename shm_ref<T>::ShmType *rhs) {
    if (rhs == m_d)
        return *this;

    unref();
    if (rhs) {
        m_name = rhs->getName();
        m_d = rhs;
    } else {
        m_name.clear();
        m_d = nullptr;
    }
    ref();
    return *this;
}

template<class T>
bool shm_ref<T>::valid() const {
    return m_name.empty() || m_d;
}

template<class T>
const typename shm_ref<T>::ShmType::Data *shm_ref<T>::getData() const {
    if (!valid())
        return nullptr;
    return &*m_d;
}

template<class T>
const shm_name_t &shm_ref<T>::name() const {
    return m_name;
}

template<class T>
void shm_ref<T>::ref() {
    if (m_d) {
        assert(m_name == m_d->name);
        assert(m_d->refcount() >= 0);
        m_d->ref();
    }
}

template<class T>
void shm_ref<T>::unref() {
    if (m_d) {
        assert(m_d->refcount() > 0);
        m_d->unref();
    }
    m_d = nullptr;
}

template<class T>
vistle::shm_ref<T>::operator bool() const {
    return valid();
}

template<class T>
template<class Archive>
void shm_ref<T>::save(Archive &ar) const {
    ar & V_NAME(ar, "shm_name", m_name);
    assert(valid());
    if (m_d)
        ar.saveObject(*this);
}

template<class T>
template<class Archive>
void shm_ref<T>::load(Archive &ar) {
    shm_name_t shmname;
    ar & V_NAME(ar, "shm_name", shmname);
    std::string arname = shmname;

    std::string name = ar.translateObjectName(arname);
    //std::cerr << "shm_ref: loading " << arname << ", translates to " << name << std::endl;
    m_name = name;

    unref();
    m_d = nullptr;

    if (arname.empty() && m_name.empty())
        return;

    auto obj = ar.currentObject();
    auto handler = ar.objectCompletionHandler();
    auto ref0 = Shm::the().getObjectFromName(name);
    auto ref1 = T::as(ref0);
    assert(ref0 || !ref1);
    if (ref1) {
        *this = ref1;
        return;
    }

    if (obj)
        obj->unresolvedReference();
    auto fetcher = ar.fetcher();
    ref0 = ar.getObject(arname, [this, fetcher, arname, name, obj, handler](Object::const_ptr newobj) -> void {
        //std::cerr << "object completion handler: " << name << std::endl;
        assert(newobj);
        auto ref2 = T::as(newobj);
        assert(ref2);
        *this = ref2;
        m_name = newobj->getName();
        if (fetcher)
            fetcher->registerObjectNameTranslation(arname, m_name);
#if 0
        auto ref2 = T::as(Shm::the().getObjectFromName(name));
        assert(ref2);
        *this = ref2;
        if (ref2) {
            m_name = ref2->getName();
            ar.registerObjectNameTranslation(arname, m_name);
        }
#endif
        if (obj) {
            obj->referenceResolved(handler);
        }
    });
    ref1 = T::as(ref0);
    assert(ref0 || !ref1);
    if (ref1) {
        m_name = ref1->getName();
        ar.registerObjectNameTranslation(arname, m_name);
        // object already present: don't mess with count of outstanding references
        *this = ref1;
    }
}
#endif







template<class T>
shm_array_ref<T>::shm_array_ref(const std::vector<typename T::value_type> &data)
    : m_name(Shm::the().createArrayId())
    , m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
    {
        ref();
        Shm::the().addArray(m_name, &*m_p);
        (*this)->resize(data.size());
        std::copy(data.begin(), data.end(), (*this)->begin());
    }

template<class T>
shm_array_ref<T>::shm_array_ref(const typename T::value_type *data, size_t size)
    : m_name(Shm::the().createArrayId())
    , m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
    {
        ref();
        Shm::the().addArray(m_name, &*m_p);
        (*this)->resize(size);
        std::copy(data, data+size, (*this)->begin());
    }

template<class T>
template<class Archive>
void shm_array_ref<T>::save(Archive &ar) const {

    ar & V_NAME(ar, "shm_name", m_name);
    ar.template saveArray<typename T::value_type>(*this);
}

template<class T>
template<class Archive>
void shm_array_ref<T>::load(Archive &ar) {
   shm_name_t shmname;
   ar & V_NAME(ar, "shm_name", shmname);

   std::string arname = shmname.str();
   std::string name = ar.translateArrayName(arname);
   //std::cerr << "shm_array_ref: loading " << shmname << " for " << m_name << "/" << name << ", valid=" << valid() << std::endl;
   if (name.empty() || m_name.str() != name) {
       unref();
       m_name.clear();
   }

   ObjectData *obj = ar.currentObject();
   if (obj) {
       //std::cerr << "obj " << obj->name << ": unresolved: " << name << std::endl;
       obj->unresolvedReference();
   }

   auto handler = ar.objectCompletionHandler();
   ar.template fetchArray<typename T::value_type>(arname, [this, obj, handler](const std::string &name) -> void {
      auto ref = Shm::the().getArrayFromName<typename T::value_type>(name);
      //std::cerr << "shm_array: array completion handler: " << arname << " -> " << name << "/" << m_name << ", ref=" << ref << std::endl;
      if (!ref) {
          std::cerr << "shm_array: NOT COMPLETE: array completion handler: " << name << ", ref=" << ref << std::endl;
      }
      assert(ref);
      *this = ref;
      if (obj) {
          //std::cerr << "obj " << obj->name << ": RESOLVED: " << name << std::endl;
          obj->referenceResolved(handler);
      } else {
          std::cerr << "shm_array RESOLVED: " << name << ", but no handler" << std::endl;
      }
   });
   //std::cerr << "shm_array: first try: this=" << *this << ", ref=" << ref << std::endl;
}

template<class T>
T &shm_array_ref<T>::operator*() {
    return *m_p;
}

template<class T>
const T &shm_array_ref<T>::operator*() const {
    return *m_p;
}

template<class T>
T *shm_array_ref<T>::operator->() {
#ifdef NO_SHMEM
    return m_p;
#else
    return m_p.get();
#endif
}

template<class T>
const T *shm_array_ref<T>::operator->() const {
#ifdef NO_SHMEM
    return m_p;
#else
    return m_p.get();
#endif
}

template<class T>
int shm_array_ref<T>::refcount() const {
    if (m_p)
        return m_p->refcount();
    return -1;
}

template<class T>
const shm_name_t &shm_array_ref<T>::name() const {
    return m_name;
}

}
#endif
