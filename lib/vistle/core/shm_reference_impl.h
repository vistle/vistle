#ifndef VISTLE_CORE_SHM_REFERENCE_IMPL_H
#define VISTLE_CORE_SHM_REFERENCE_IMPL_H

#include "archives_config.h"
#include "object.h"

namespace vistle {

template<class T>
shm_array_ref<T>::shm_array_ref(const std::vector<typename T::value_type> &data)
: m_name(Shm::the().createArrayId()), m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
{
    ref();
    Shm::the().addArray(m_name, &*m_p);
    (*this)->resize(data.size());
    std::copy(data.begin(), data.end(), (*this)->begin());
}

template<class T>
shm_array_ref<T>::shm_array_ref(
    const std::vector<typename T::value_type, vistle::default_init_allocator<typename T::value_type>> &data)
: m_name(Shm::the().createArrayId()), m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
{
    ref();
    Shm::the().addArray(m_name, &*m_p);
    (*this)->resize(data.size());
    std::copy(data.begin(), data.end(), (*this)->begin());
}

template<class T>
shm_array_ref<T>::shm_array_ref(const typename T::value_type *data, size_t size)
: m_name(Shm::the().createArrayId()), m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
{
    ref();
    Shm::the().addArray(m_name, &*m_p);
    (*this)->resize(size);
    std::copy(data, data + size, (*this)->begin());
}

template<class T>
template<class Archive>
void shm_array_ref<T>::save(Archive &ar) const
{
    ar &V_NAME(ar, "shm_name", m_name);
    ar.template saveArray<typename T::value_type>(*this);
}

template<class T>
template<class Archive>
void shm_array_ref<T>::load(Archive &ar)
{
    shm_name_t shmname;
    ar &V_NAME(ar, "shm_name", shmname);

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
        obj->unresolvedReference(true, arname, name);
    }

    auto handler = ar.objectCompletionHandler();
    ar.template fetchArray<typename T::value_type>(
        arname, [this, obj, handler, arname](const std::string &name) -> void {
            auto ref = Shm::the().getArrayFromName<typename T::value_type>(name);
            //std::cerr << "shm_array: array completion handler: " << arname << " -> " << name << "/" << m_name << ", ref=" << ref << std::endl;
            if (!ref) {
                std::cerr << "shm_array: NOT COMPLETE: array completion handler: " << name << ", ref=" << ref
                          << std::endl;
            }
            assert(ref);
            *this = ref;
            if (obj) {
                //std::cerr << "obj " << obj->name << ": RESOLVED: " << name << std::endl;
                obj->referenceResolved(handler, true, arname, name);
            } else {
                std::cerr << "shm_array RESOLVED: " << name << ", but no handler" << std::endl;
            }
        });
    //std::cerr << "shm_array: first try: this=" << *this << ", ref=" << ref << std::endl;
}

template<class T>
T &shm_array_ref<T>::operator*()
{
    return *m_p;
}

template<class T>
const T &shm_array_ref<T>::operator*() const
{
    return *m_p;
}

template<class T>
T *shm_array_ref<T>::operator->()
{
#ifdef NO_SHMEM
    return m_p;
#else
    return m_p.get();
#endif
}

template<class T>
const T *shm_array_ref<T>::operator->() const
{
#ifdef NO_SHMEM
    return m_p;
#else
    return m_p.get();
#endif
}

template<class T>
int shm_array_ref<T>::refcount() const
{
    if (m_p)
        return m_p->refcount();
    return -1;
}

template<class T>
const shm_name_t &shm_array_ref<T>::name() const
{
    return m_name;
}

template<class T>
void shm_array_ref<T>::print(std::ostream &os, bool verbose) const
{
    os << m_name << "(";
    if (m_p) {
        m_p->print(os, verbose);
    }
    os << ")";
}

} // namespace vistle
#endif
