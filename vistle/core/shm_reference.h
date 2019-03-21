#ifndef SHM_REFRENCE_H
#define SHM_REFRENCE_H

#include "archives_config.h"
#include <string>
#include <cassert>
#include "shmname.h"

namespace vistle {

template<class T>
class shm_ref {

 public:
    shm_ref()
    //: m_name(Shm::the().createArrayId())
    : m_name("")
    //, m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
    , m_p(nullptr)
    {
        ref();
    }

    shm_ref(const std::string &name, T *p)
    : m_name(name)
    , m_p(p)
    {
        ref();
    }

    shm_ref(const shm_ref &other)
    : m_name(other.m_name)
    , m_p(other.m_p)
    {
        ref();
    }

    shm_ref(const shm_name_t name)
    : m_name(name)
    , m_p(shm<T>::find(name))
    {
        ref();
    }

   ~shm_ref() {
        unref();
    }

    template<typename... Args>
    static shm_ref create(const Args&... args) {
       shm_ref result;
       result.construct(args...);
       return result;
    }

    bool find() {
        assert(!m_name.empty());
        if (!m_p) {
            m_p = shm<T>::find(m_name);
            ref();
        }
        return valid();
    }

    template<typename... Args>
    void construct(const Args&... args)
    {
        unref();
        if (m_name.empty())
            m_name = Shm::the().createArrayId();
        assert(!m_name.empty());
        m_p = shm<T>::construct(m_name)(args..., Shm::the().allocator());
        ref();
    }

    const shm_ref &operator=(const shm_ref &rhs) {
        unref();
        m_name = rhs.m_name;
        m_p = rhs.m_p;
        ref();
        return *this;
    }

   bool valid() const {
       return !!m_p;
   }

   operator bool() const {
       return valid();
   }

   T &operator*();
   const T &operator*() const;

   T *operator->();
   const T *operator->() const;

   const shm_name_t &name() const;

 private:
   shm_name_t m_name;
#ifdef NO_SHMEM
   T *m_p;
#else
   boost::interprocess::offset_ptr<T> m_p;
#endif
   void ref() {
       if (m_p) {
           assert(!m_name.empty());
           assert(m_p->refcount() >= 0);
           m_p->ref();
       }
   }

   void unref() {
       if (m_p) {
            assert(!m_name.empty());
            assert(m_p->refcount() > 0);
            if (m_p->unref() == 0) {
                shm<T>::destroy(m_name);
                m_p = nullptr;
            }
       }
   }

   ARCHIVE_ACCESS_SPLIT

   template<class Archive>
   void save(Archive &ar) const;
   template<class Archive>
   void load(Archive &ar);
};

#define V_DECLARE_SHMREF(T) \
    extern template class V_COREEXPORT shm_ref<shm_array<T, typename shm<T>::allocator>>;

#define V_DEFINE_SHMREF(T) \
    template class shm_ref<shm_array<T, typename shm<T>::allocator>>; \
    template void shm_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::yas_iarchive>(vistle::yas_iarchive &ar); \
    template void shm_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::boost_iarchive>(vistle::boost_iarchive &ar); \
    template void shm_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::yas_oarchive>(vistle::yas_oarchive &ar) const; \
    template void shm_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::boost_oarchive>(vistle::boost_oarchive &ar) const;

V_DECLARE_SHMREF(unsigned char)
V_DECLARE_SHMREF(int32_t)
V_DECLARE_SHMREF(uint32_t)
V_DECLARE_SHMREF(int64_t)
V_DECLARE_SHMREF(uint64_t)
V_DECLARE_SHMREF(float)
V_DECLARE_SHMREF(double)


} // namespace vistle
#endif
