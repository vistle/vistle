#ifndef VISTLE_SHM_OBJ_REF_IMPL_H
#define VISTLE_SHM_OBJ_REF_IMPL_H

#include "object.h"
#include "shm.h"
#include <cassert>

namespace vistle {

template<class T>
shm_obj_ref<T>::shm_obj_ref()
    : m_name()
    , m_d(nullptr)
{
    ref();
}

template<class T>
shm_obj_ref<T>::shm_obj_ref(const std::string &name, shm_obj_ref<T>::ObjType *p)
    : m_name(name)
    , m_d(p->d())
{
    ref();
}

template<class T>
shm_obj_ref<T>::shm_obj_ref(const shm_obj_ref<T> &other)
    : m_name(other.m_name)
    , m_d(other.m_d)
{
    ref();
}

template<class T>
shm_obj_ref<T>::shm_obj_ref(const shm_name_t name)
    : m_name(name)
    , m_d(shm<T>::find(name))
{
    ref();
}

template<class T>
shm_obj_ref<T>::~shm_obj_ref() {
    unref();
}

template<class T>
template<typename... Args>
shm_obj_ref<T> shm_obj_ref<T>::create(const Args&... args) {
    shm_obj_ref<T> result;
    result.construct(args...);
    return result;
}

template<class T>
bool shm_obj_ref<T>::find() {
    auto mem = vistle::shm<ObjData>::find(m_name);
    m_d = mem;
    ref();

    return valid();
}

template<class T>
template<typename... Args>
void shm_obj_ref<T>::construct(const Args&... args)
{
    unref();
    if (m_name.empty())
        m_name = Shm::the().createObjectId();
    m_d = shm<T>::construct(m_name)(args..., Shm::the().allocator());
    ref();
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(const shm_obj_ref<T> &rhs) {
    unref();
    m_name = rhs.m_name;
    m_d = rhs.m_d;
    ref();
    return *this;
}

template<class T>
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(typename shm_obj_ref<T>::ObjType::const_ptr rhs) {
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
const shm_obj_ref<T> &shm_obj_ref<T>::operator=(typename shm_obj_ref<T>::ObjType::ptr rhs) {
    return std::const_pointer_cast<const ObjType>(rhs);
}

template<class T>
bool shm_obj_ref<T>::valid() const {
    return m_name.empty() || m_d;
}

template<class T>
typename shm_obj_ref<T>::ObjType::const_ptr shm_obj_ref<T>::getObject() const {
    if (!valid())
        return nullptr;
    return ObjType::as(Object::create(const_cast<typename ObjType::Data *>(&*m_d)));
}

template<class T>
const typename shm_obj_ref<T>::ObjType::Data *shm_obj_ref<T>::getData() const {
    if (!valid())
        return nullptr;
    return &*m_d;
}

template<class T>
const shm_name_t &shm_obj_ref<T>::name() const {
    return m_name;
}

template<class T>
void shm_obj_ref<T>::ref() {
    if (m_d) {
        assert(m_name == m_d->name);
        assert(m_d->refcount >= 0);
        m_d->ref();
    }
}

template<class T>
void shm_obj_ref<T>::unref() {
    if (m_d) {
        assert(m_d->refcount > 0);
        m_d->unref();
    }
    m_d = nullptr;
}

template<class T>
vistle::shm_obj_ref<T>::operator bool() const {
    return valid();
}

template<class T>
template<class Archive>
void shm_obj_ref<T>::save(Archive &ar) const {
    ar & V_NAME(ar, "obj_name", m_name);
    assert(valid());
    if (m_d)
        ar.saveObject(*this);
}

template<class T>
template<class Archive>
void shm_obj_ref<T>::load(Archive &ar) {
    ar & V_NAME(ar, "obj_name", m_name);

    //assert(shmname == m_name);
    std::string name = m_name;

    unref();
    m_d = nullptr;

    if (m_name.empty())
        return;

    auto obj = ar.currentObject();
    auto handler = ar.objectCompletionHandler();
    auto ref0 = Shm::the().getObjectFromName(name);
    auto ref1 = T::as(ref0);
    assert(ref0 || !ref1);
    if (ref1) {
        *this = ref1;
    } else {
        if (obj) {
            obj->unresolvedReference();
        }
    }
    ref0 = ar.getObject(name, [this, name, obj, handler]() -> void {
        //std::cerr << "object completion handler: " << name << std::endl;
        auto ref2 = T::as(Shm::the().getObjectFromName(name));
        assert(ref2);
        *this = ref2;
        if (obj) {
            obj->referenceResolved(handler);
        }
    });
    ref1 = T::as(ref0);
    assert(ref0 || !ref1);
    if (ref1) {
        // object already present: don't mess with count of outstanding references
        *this = ref1;
    }
}

}
#endif
