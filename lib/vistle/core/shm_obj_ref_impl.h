#ifndef VISTLE_CORE_SHM_OBJ_REF_IMPL_H
#define VISTLE_CORE_SHM_OBJ_REF_IMPL_H

#include "object.h"
#include "shm.h"
#include <cassert>

namespace vistle {

template<class T>
shm_obj_ref<T>::shm_obj_ref(): m_name(), m_d(nullptr)
{
    ref();
}

template<class T>
shm_obj_ref<T>::shm_obj_ref(const std::string &name, shm_obj_ref<T>::ObjType *p): m_name(name), m_d(p->d())
{
    ref();
}

template<class T>
shm_obj_ref<T>::shm_obj_ref(const shm_obj_ref<T> &other): m_name(other.m_name), m_d(other.m_d)
{
    ref();
}

template<class T>
shm_obj_ref<T>::~shm_obj_ref()
{
    unref();
}

template<class T>
template<typename... Args>
shm_obj_ref<T> shm_obj_ref<T>::create(const Args &...args)
{
    shm_obj_ref<T> result;
    result.construct(args...);
    return result;
}

template<class T>
bool shm_obj_ref<T>::find()
{
    auto mem = vistle::shm<ObjData>::find(m_name);
    m_d = mem;
    ref();

    return valid();
}

template<class T>
template<typename... Args>
void shm_obj_ref<T>::construct(const Args &...args)
{
    unref();
    if (m_name.empty())
        m_name = Shm::the().createObjectId();
    m_d = shm<T>::construct(m_name)(args..., Shm::the().allocator());
    ref();
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(const shm_obj_ref<T> &rhs)
{
    if (&rhs == this)
        return *this;
    unref();
    m_name = rhs.m_name;
    m_d = rhs.m_d;
    ref();
    return *this;
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(const typename shm_obj_ref<T>::ObjType::const_ptr &rhs)
{
    unref();
    if (rhs) {
        m_name = rhs->getName();
        m_d = rhs->d();
    } else {
        m_name.clear();
        m_d = nullptr;
    }
    ref();
    return *this;
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(const typename shm_obj_ref<T>::ObjType::ptr &rhs)
{
    // reuse operator for ObjType::const_ptr
    *this = std::const_pointer_cast<const ObjType>(rhs);
    return *this;
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(const typename shm_obj_ref<T>::ObjType::Data *rhs)
{
    unref();
    if (rhs) {
        m_name = rhs->name;
        m_d = rhs;
    } else {
        m_name.clear();
        m_d = nullptr;
    }
    ref();
    return *this;
}

template<class T>
bool shm_obj_ref<T>::valid() const
{
    return m_name.empty() || m_d;
}

template<class T>
typename T::const_ptr shm_obj_ref<T>::getObject() const
{
    if (!valid())
        return nullptr;
    typedef typename shm_obj_ref<T>::ObjType ObjType;
    return typename ObjType::const_ptr(
        dynamic_cast<ObjType *>(Object::create(const_cast<typename ObjType::Data *>(&*m_d))));
}

template<class T>
const typename T::Data *shm_obj_ref<T>::getData() const
{
    if (!valid())
        return nullptr;
    return &*m_d;
}

template<class T>
const shm_name_t &shm_obj_ref<T>::name() const
{
    return m_name;
}

template<class T>
void shm_obj_ref<T>::ref()
{
    if (m_d) {
        assert(m_name == m_d->name);
        assert(m_d->refcount() >= 0);
        m_d->ref();
    }
}

template<class T>
void shm_obj_ref<T>::unref()
{
    if (m_d) {
        assert(m_d->refcount() > 0);
        m_d->unref();
    }
    m_d = nullptr;
}

template<class T>
vistle::shm_obj_ref<T>::operator bool() const
{
    return valid();
}

template<class T>
template<class Archive>
void shm_obj_ref<T>::save(Archive &ar) const
{
    ar &V_NAME(ar, "obj_name", m_name);
    assert(valid());
    if (m_d)
        ar.saveObject(*this);
}

template<class T>
template<class Archive>
void shm_obj_ref<T>::load(Archive &ar)
{
    shm_name_t shmname;
    ar &V_NAME(ar, "obj_name", shmname);
    std::string arname = shmname;

    m_name = ar.translateObjectName(arname);
    //std::cerr << "shm_obj_ref: loading " << arname << ", translates to " << m_name << std::endl;

    unref();
    m_d = nullptr;

    if (arname.empty() && m_name.empty()) {
        // null reference
        return;
    }

    auto ref0 = Shm::the().getObjectFromName(m_name);
    auto ref1 = T::as(ref0);
    assert(ref0 || !ref1);
    if (ref1) {
        // found, and types do match
        *this = ref1;
        return;
    }

    //
    auto obj = ar.currentObject();
    if (obj)
        obj->unresolvedReference(false, arname, m_name.str());
    auto handler = ar.objectCompletionHandler();
    auto fetcher = ar.fetcher();
    ref0 = ar.getObject(arname, [this, fetcher, arname, obj, handler](Object::const_ptr newobj) -> void {
        assert(newobj);
        auto ref2 = T::as(newobj);
        assert(ref2);
        *this = ref2;
        m_name = newobj->getName();
        //std::cerr << "object completion handler: " << m_name << " (in archive as " << arname << ")" << std::endl;
        if (fetcher)
            fetcher->registerObjectNameTranslation(arname, m_name);
        if (obj) {
            obj->referenceResolved(handler, false, arname, m_name.str());
        }
    });
    ref1 = T::as(ref0);
    if (ref1) {
        // loaded, and types do match
        *this = ref1;
    }
}

} // namespace vistle
#endif
