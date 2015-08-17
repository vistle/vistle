#ifndef SHM_REFRENCE_H
#define SHM_REFRENCE_H

//#include "shm.h"
#include <string>

namespace vistle {

template<class T>
class shm_ref: public T {

 public:
#if 1
    shm_ref()
    : m_name(Shm::the().createObjectId())
    , m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
    {
        ref();
    }

    template<typename... Args>
    shm_ref(const Args&... args)
    : m_name(Shm::the().createObjectId())
    , m_p(shm<T>::construct(m_name)(args..., Shm::the().allocator()))
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

   ~shm_ref();

    template<typename... Args>
    void construct(const Args&... args)
    {
        unref();
        m_p = shm<T>::construct(m_name)(args..., Shm::the().allocator());
        ref();
    }

    const shm_ref &operator=(const shm_ref &rhs) {
        unref();
        m_p = rhs.m_p;
        ref();
    }

#else
   shm_ref(T *t, const std::string &name = "");
   shm_ref(const std::string &name);
#endif

   bool valid() const {
       return m_p;
   }

   operator bool() const {
       return valid();
   }

   T &operator*();
   const T &operator*() const;

   T *operator->();
   const T *operator->() const;

   shm_name_t &name() const;

 private:
   shm_name_t m_name;
   boost::interprocess::offset_ptr<T> m_p;
   void ref() {
       if (m_p)
           m_p->ref();
   }

   void unref() {
       if (m_p) {
            if (m_p->unref() == 0) {
                shm<T>::destroy(m_p);
            }
       }
   }
};

} // namespace vistle

#include "shm_reference_impl.h"

#endif
