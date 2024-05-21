#ifndef SHM_ARRAY_H
#define SHM_ARRAY_H

#include <cassert>
#include <atomic>
#include <cstring>
#include <type_traits>
#include <ostream>

#include "export.h"
#include "index.h"
//#include "archives_config.h"
#include "shmdata.h"
#include "shm_config.h"
#include "scalars.h"
#include "celltreenode_decl.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ArrayCopy.h>

#include <vistle/util/profile.h>

namespace vistle {

template<typename T, class allocator>
class shm_array;

template<typename T>
struct ArrayHandleTypeMap {
    typedef T type;
};

// map from unsigned Index types to signed vtkm::Id's
template<>
struct ArrayHandleTypeMap<Index> {
    typedef vtkm::Id type;
    static_assert(sizeof(Index) == sizeof(vtkm::Id));
};

template<typename T, class allocator>
std::ostream &operator<<(std::ostream &os, const shm_array<T, allocator> &arr);

template<typename T, class allocator>
class shm_array: public ShmData {
    friend std::ostream &operator<< <T, allocator>(std::ostream &os, const shm_array<T, allocator> &arr);

public:
    typedef T value_type;
    typedef typename ArrayHandleTypeMap<T>::type handle_type;
    typedef uint64_t size_type;
    typedef const value_type &const_reference;

    static unsigned typeId();

    shm_array(const allocator &alloc);
    shm_array(size_t size, const allocator &alloc);
    shm_array(size_t size, const T &value, const allocator &alloc);
    shm_array(shm_array &&other);
    ~shm_array();

    unsigned type() const { return m_type; }
    bool check(std::ostream &os) const;

    template<class ArrayHandle>
    void setHandle(const ArrayHandle &handle);

#ifdef NO_SHMEM
    const vtkm::cont::ArrayHandle<handle_type> &handle() const;
#else
    const vtkm::cont::ArrayHandle<handle_type> handle() const;
#endif
    void updateFromHandle(bool invalidate = false);
    void updateFromHandle(bool invalidate = false) const;

    typedef typename std::allocator_traits<allocator>::pointer pointer;
    typedef T *iterator;
    typedef const T *const_iterator;

    const iterator begin() const
    {
        updateFromHandle();
        return &*m_data;
    }
    const iterator end() const
    {
        updateFromHandle();
        return (&*m_data) + m_size;
    }
    iterator begin()
    {
        updateFromHandle(true);
        return &*m_data;
    }
    iterator end()
    {
        updateFromHandle(true);
        return (&*m_data) + m_size;
    }

    T *data()
    {
        updateFromHandle(true);
        return &*m_data;
    }
    const T *data() const
    {
        updateFromHandle();
        return &*m_data;
    }

    T &operator[](const size_t idx)
    {
        updateFromHandle(true);
        return m_data[idx];
    }
    const T &operator[](const size_t idx) const
    {
        updateFromHandle();
        return m_data[idx];
    }

    T &at(const size_t idx);
    const T &at(const size_t idx) const;

    void push_back(const T &v);

    template<class... Args>
    void emplace_back(Args &&...args)
    {
        updateFromHandle(true);
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 1 : m_capacity * 2);
        assert(m_size < m_capacity);
        new (&m_data[m_size]) T(std::forward<Args>(args)...);
        ++m_size;
    }

    T &back()
    {
        updateFromHandle(true);
        return m_data[m_size - 1];
    }
    T &front()
    {
        updateFromHandle(true);
        return m_data[0];
    }

    const T &back() const
    {
        updateFromHandle();
        return m_data[m_size - 1];
    }
    const T &front() const
    {
        updateFromHandle();
        return m_data[0];
    }
    bool empty() const
    {
        return m_size == 0;
    }
    void clear();

    size_t size() const
    {
        return m_size;
    }
    void resize(const size_t size);
    void resize(const size_t size, const T &value);

    void clearDimensionHint();
    void setDimensionHint(const size_t sx, const size_t sy = 1, const size_t sz = 1);
    void setExact(bool exact);

    size_t capacity() const
    {
        return m_capacity;
    }
    void reserve(const size_t new_capacity);
    void reserve_or_shrink(const size_t capacity);
    void shrink_to_fit();

    bool bounds_valid() const
    {
        return m_max >= m_min;
    }
    void invalidate_bounds();
    void update_bounds();

    value_type min() const
    {
        assert(bounds_valid());
        return m_min;
    }

    value_type max() const
    {
        assert(bounds_valid());
        return m_max;
    }

    void print(std::ostream &os, bool verbose = false) const;

private:
    const uint32_t m_type;
    size_t m_size = 0;
    size_t m_dim[3] = {0, 1, 1};
#ifdef NO_SHMEM
    mutable size_t m_capacity = 0;
#else
    size_t m_capacity = 0;
#endif
    bool m_exact = std::is_integral<T>::value;
    value_type m_min = std::numeric_limits<value_type>::max();
    value_type m_max = std::numeric_limits<value_type>::lowest();
#ifdef NO_SHMEM
    mutable pointer m_data = nullptr;
    mutable std::atomic<bool> m_memoryValid = true;
    mutable std::mutex m_mutex;
    vtkm::cont::UnknownArrayHandle m_unknown;
    mutable vtkm::cont::ArrayHandleBasic<handle_type> m_handle; // VTK-m ArrayHandle that is always kept up to date
#else
    pointer m_data = nullptr;
    allocator m_allocator;
#endif

    ARCHIVE_ACCESS_SPLIT
    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);

    // not implemented
    shm_array(const shm_array &other) = delete;
    shm_array &operator=(const shm_array &rhs) = delete;
};

template<typename T, class allocator>
void shm_array<T, allocator>::updateFromHandle(bool invalidate)
{
#ifdef NO_SHMEM
    if (m_memoryValid && !invalidate)
        return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_memoryValid) {
        PROF_SCOPE("shm_array::updateFromHandle()");
        m_memoryValid = true;

        if (m_unknown.CanConvert<vtkm::cont::ArrayHandleBasic<handle_type>>()) {
            m_handle = m_unknown.AsArrayHandle<vtkm::cont::ArrayHandleBasic<handle_type>>();
        } else {
            vtkm::cont::ArrayCopy(m_unknown, m_handle);
        }
        m_data = reinterpret_cast<T *>(m_handle.GetWritePointer());
    }
    if (invalidate) {
        m_unknown = vtkm::cont::UnknownArrayHandle();
    }
#endif
}

template<typename T, class allocator>
void shm_array<T, allocator>::updateFromHandle(bool invalidate) const
{
#ifdef NO_SHMEM
    assert(!invalidate);
    if (m_memoryValid)
        return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_memoryValid)
        return;
    m_memoryValid = true;

    PROF_SCOPE("shm_array::updateFromHandle()const");
    if (m_unknown.CanConvert<vtkm::cont::ArrayHandleBasic<handle_type>>()) {
        m_handle = m_unknown.AsArrayHandle<vtkm::cont::ArrayHandleBasic<handle_type>>();
    } else {
        vtkm::cont::ArrayCopy(m_unknown, m_handle);
    }
    m_data = reinterpret_cast<T *>(m_handle.GetWritePointer());
#endif
}

#define SHMARR_EXPORT(T) \
    extern template class V_COREEXPORT shm_array<T, shm_allocator<T>>; \
    extern template V_COREEXPORT std::ostream &operator<< <T, shm_allocator<T>>( \
        std::ostream &, const shm_array<T, shm_allocator<T>> &);

FOR_ALL_SCALARS(SHMARR_EXPORT)
} // namespace vistle

#include "celltreenode.h"
namespace vistle {
SHMARR_EXPORT(CelltreeNode1)
SHMARR_EXPORT(CelltreeNode2)
SHMARR_EXPORT(CelltreeNode3)
#undef SHMARR_EXPORT

} // namespace vistle
#endif
