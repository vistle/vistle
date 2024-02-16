#ifndef SHM_REFRENCE_H
#define SHM_REFRENCE_H

#include "archives_config.h"
#include <string>
#include <cassert>
#include "shmname.h"
#include "shm_array.h"
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/offset_ptr.hpp>

namespace vistle {

template<class T>
class shm_array_ref {
public:
    shm_array_ref()
    //: m_name(Shm::the().createArrayId())
    : m_name("")
    //, m_p(shm<T>::construct(m_name)(Shm::the().allocator()))
    , m_p(nullptr)
    {
        ref();
    }

    shm_array_ref(const std::string &name, T *p): m_name(name), m_p(p) { ref(); }

    shm_array_ref(const shm_array_ref &other): m_name(other.m_name), m_p(other.m_p)
    {
        if (m_p) {
            assert(other->refcount() > 0);
        }
        ref();
    }

    explicit shm_array_ref(const shm_name_t name)
    : m_name(name), m_p(name.empty() ? nullptr : shm<T>::find_and_ref(name))
    {
        assert(!m_p || refcount() > 0);
    }

    explicit shm_array_ref(const std::vector<typename T::value_type> &data);
    explicit shm_array_ref(
        const std::vector<typename T::value_type, vistle::default_init_allocator<typename T::value_type>> &data);
    shm_array_ref(const typename T::value_type *data, size_t size);

    ~shm_array_ref()
    {
        if (m_p) {
            assert(refcount() > 0);
        }
        unref();
    }

    void reset()
    {
        if (m_p) {
            assert(refcount() > 0);
        }
        unref();
        m_p = nullptr;
        m_name = std::string();
    }

    template<typename... Args>
    static shm_array_ref create(const Args &...args)
    {
        shm_array_ref result;
        result.construct(args...);
        return result;
    }

    bool find()
    {
        assert(!m_name.empty());
        if (!m_p) {
            m_p = shm<T>::find_and_ref(m_name);
        }
        if (m_p) {
            assert(refcount() > 0);
        }
        return valid();
    }

    template<typename... Args>
    void construct(const Args &...args)
    {
        assert(!valid());
        unref();
        if (m_name.empty())
            m_name = Shm::the().createArrayId();
        assert(!m_name.empty());
        m_p = shm<T>::construct(m_name)(args..., Shm::the().allocator());
        ref();
        assert(m_p->refcount() == 1);
        Shm::the().addArray(m_name, &*m_p);
    }

    const shm_array_ref &operator=(const shm_array_ref &rhs)
    {
        if (&rhs != this) {
            unref();
            m_name = rhs.m_name;
            m_p = rhs.m_p;
            ref();
        }
        return *this;
    }

    bool valid() const { return !!m_p; }

    operator bool() const { return valid(); }

    T &operator*();
    const T &operator*() const;

    T *operator->();
    const T *operator->() const;

    const shm_name_t &name() const;
    int refcount() const;

    void ref()
    {
        if (m_p) {
            assert(!m_name.empty());
            assert(m_p->refcount() >= 0);
            m_p->ref();
        }
    }

    void unref()
    {
        if (m_p) {
            assert(!m_name.empty());
            assert(m_p->refcount() > 0);
            //std::cerr << "shm_array_ref: giving up reference to " << m_name << ", refcount=" << m_p->refcount() << std::endl;
            if (m_p->unref() == 0) {
                shm<typename T::value_type>::destroy_array(m_name, m_p);
                m_p = nullptr;
            }
        }
    }

private:
    shm_name_t m_name;
#ifdef NO_SHMEM
    T *m_p;
#else
    boost::interprocess::offset_ptr<T> m_p;
#endif
    ARCHIVE_ACCESS_SPLIT

    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);
};


#ifdef USE_BOOST_ARCHIVE
#ifdef USE_YAS
#define V_DECLARE_SHMREF(T) \
    extern template class V_COREEXPORT shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::yas_iarchive>(vistle::yas_iarchive & \
                                                                                            ar); \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::boost_iarchive>(vistle::boost_iarchive & \
                                                                                              ar); \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::yas_oarchive>(vistle::yas_oarchive & ar) \
            const; \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::boost_oarchive>(vistle::boost_oarchive & \
                                                                                              ar) const;

#define V_DEFINE_SHMREF(T) \
    template class shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::yas_iarchive>( \
        vistle::yas_iarchive & ar); \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::boost_iarchive>( \
        vistle::boost_iarchive & ar); \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::yas_oarchive>( \
        vistle::yas_oarchive & ar) const; \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::boost_oarchive>( \
        vistle::boost_oarchive & ar) const;
#else
#define V_DECLARE_SHMREF(T) \
    extern template class V_COREEXPORT shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::boost_iarchive>(vistle::boost_iarchive & \
                                                                                              ar); \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::boost_oarchive>(vistle::boost_oarchive & \
                                                                                              ar) const;

#define V_DEFINE_SHMREF(T) \
    template class shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::boost_iarchive>( \
        vistle::boost_iarchive & ar); \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::boost_oarchive>( \
        vistle::boost_oarchive & ar) const;
#endif
#else
#define V_DECLARE_SHMREF(T) \
    extern template class V_COREEXPORT shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::yas_iarchive>(vistle::yas_iarchive & \
                                                                                            ar); \
    extern template void V_COREEXPORT \
        shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::yas_oarchive>(vistle::yas_oarchive & ar) \
            const;

#define V_DEFINE_SHMREF(T) \
    template class shm_array_ref<shm_array<T, typename shm<T>::allocator>>; \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::load<vistle::yas_iarchive>( \
        vistle::yas_iarchive & ar); \
    template void shm_array_ref<shm_array<T, typename shm<T>::allocator>>::save<vistle::yas_oarchive>( \
        vistle::yas_oarchive & ar) const;
#endif

FOR_ALL_SCALARS(V_DECLARE_SHMREF)

template<typename T>
class ShmArrayProxy {
public:
    typedef typename ArrayHandleTypeMap<T>::type handle_type;

    ShmArrayProxy() = default;

    template<class allocator>
    ShmArrayProxy(const shm_array<T, allocator> *arr)
    : m_data(arr ? arr->data() : nullptr)
    , m_size(arr ? arr->size() : nullptr)
    , m_handle(
          arr ? arr->handle()
              : vtkm::cont::make_ArrayHandle<handle_type>(static_cast<handle_type *>(nullptr), 0, vtkm::CopyFlag::Off))
    {}

    ShmArrayProxy &operator=(std::nullptr_t p)
    {
        m_data = nullptr;
        m_size = 0;
        m_handle =
            vtkm::cont::make_ArrayHandle<handle_type>(static_cast<handle_type *>(nullptr), 0, vtkm::CopyFlag::Off);
        return *this;
    }

    template<class allocator>
    ShmArrayProxy &operator=(const shm_array<T, allocator> *arr)
    {
        if (arr) {
            m_data = arr->data();
            m_size = arr->size();
            m_handle = arr->handle();
        } else {
            m_data = nullptr;
            m_size = 0;
            m_handle = vtkm::cont::make_ArrayHandle<T>(static_cast<handle_type *>(nullptr), 0, vtkm::CopyFlag::Off);
        }
        return *this;
    }

    template<class Arr>
    ShmArrayProxy &operator=(const shm_array_ref<Arr> ref)
    {
        if (ref.valid()) {
            m_data = ref->data();
            m_size = ref->size();
            m_handle = ref->handle();
        } else {
            m_data = nullptr;
            m_size = 0;
            m_handle =
                vtkm::cont::make_ArrayHandle<handle_type>(static_cast<handle_type *>(nullptr), 0, vtkm::CopyFlag::Off);
        }
        return *this;
    }

    operator const T *() const { return m_data; }

    const T &operator*() const { return m_data[0]; }
    const T &operator[](size_t idx) const { return m_data[idx]; }
    const T *data() const { return m_data; }
    const size_t size() const { return m_size; }
    const vtkm::cont::ArrayHandle<handle_type> &handle() const { return m_handle; }

private:
    const T *m_data = nullptr;
    size_t m_size = 0;
    vtkm::cont::ArrayHandle<handle_type> m_handle;
};


} // namespace vistle
#endif
