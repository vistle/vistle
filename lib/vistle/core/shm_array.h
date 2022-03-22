#ifndef SHM_ARRAY_H
#define SHM_ARRAY_H

#include <cassert>
#include <atomic>
#include <cstring>
#include <type_traits>

#include <vistle/util/exception.h>
#include <vistle/util/tools.h>
#include "export.h"
#include "index.h"
#include "archives_config.h"
#include "shmdata.h"

namespace vistle {

template<typename T, class allocator>
class shm_array: public ShmData {
public:
    typedef T value_type;
    typedef uint64_t size_type;
    typedef const value_type &const_reference;

    static int typeId();

    shm_array(const allocator &alloc = allocator())
    : ShmData(ShmData::ARRAY), m_type(typeId()), m_size(0), m_capacity(0), m_data(nullptr), m_allocator(alloc)
    {
        resize(0);
    }

    shm_array(const size_t size, const allocator &alloc = allocator())
    : ShmData(ShmData::ARRAY), m_type(typeId()), m_size(0), m_capacity(0), m_data(nullptr), m_allocator(alloc)
    {
        resize(size);
    }

    shm_array(const size_t size, const T &value, const allocator &alloc = allocator())
    : ShmData(ShmData::ARRAY), m_type(typeId()), m_size(0), m_capacity(0), m_data(nullptr), m_allocator(alloc)
    {
        resize(size, value);
    }

    shm_array(shm_array &&other)
    : ShmData(ShmData::ARRAY)
    , m_type(other.m_type)
    , m_size(other.m_size)
    , m_capacity(other.m_capacity)
    , m_min(other.m_min)
    , m_max(other.m_max)
    , m_data(other.m_data)
    , m_allocator(other.m_allocator)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
        assert(typeId() == m_type);
    }

    ~shm_array()
    {
        assert(refcount() == 0);
        reserve_or_shrink(0);
    }

    int type() const { return m_type; }
    bool check() const
    {
        assert(refcount() >= 0);
        assert(m_size <= m_capacity);
        if (refcount() < 0) {
            std::cerr << "shm_array: INCONSISTENCY: refcount() < 0" << std::endl;
            return false;
        }
        if (m_size > m_capacity) {
            std::cerr << "shm_array: INCONSISTENCY: m_size > m_capacity" << std::endl;
            return false;
        }
        if (m_dim[0] != 0 && m_size != m_dim[0] * m_dim[1] * m_dim[2]) {
            std::cerr << "shm_array: INCONSISTENCY: dimensions" << std::endl;
            return false;
        }
        return true;
    }

    typedef typename allocator::pointer pointer;
    typedef T *iterator;
    typedef const T *const_iterator;

    iterator begin() const { return &*m_data; }
    iterator end() const { return (&*m_data) + m_size; }
    T *data() const { return &*m_data; }

    T &operator[](const size_t idx) { return m_data[idx]; }
    T &operator[](const size_t idx) const { return m_data[idx]; }
    T &at(const size_t idx)
    {
        if (idx >= m_size)
            throw(std::out_of_range("shm_array"));
        return m_data[idx];
    }
    T &at(const size_t idx) const
    {
        if (idx >= m_size)
            throw(std::out_of_range("shm_array"));
        return m_data[idx];
    }

    void push_back(const T &v)
    {
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 1 : m_capacity * 2);
        assert(m_size < m_capacity);
        new (&m_data[m_size]) T(v);
        ++m_size;
    }

    template<class... Args>
    void emplace_back(Args &&...args)
    {
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 1 : m_capacity * 2);
        assert(m_size < m_capacity);
        new (&m_data[m_size]) T(std::forward<Args>(args)...);
        ++m_size;
    }

    T &back() { return m_data[m_size - 1]; }
    T &front() { return m_data[0]; }

    bool empty() const { return m_size == 0; }
    void clear() { resize(0); }

    size_t size() const { return m_size; }
    void resize(const size_t size)
    {
        reserve(size);
        if (!std::is_trivially_copyable<T>::value) {
            for (size_t i = m_size; i < size; ++i)
                new (&m_data[i]) T();
        }
        m_size = size;
        assert(m_size <= m_capacity);
        clearDimensionHint();
    }
    void resize(const size_t size, const T &value)
    {
        reserve(size);
        for (size_t i = m_size; i < size; ++i)
            new (&m_data[i]) T(value);
        m_size = size;
        assert(m_size <= m_capacity);
        clearDimensionHint();
    }

    void clearDimensionHint()
    {
        m_dim[0] = 0;
        m_dim[1] = m_dim[2] = 1;
    }
    void setDimensionHint(const size_t sx, const size_t sy = 1, const size_t sz = 1)
    {
        assert(m_size == sx * sy * sz);
        m_dim[0] = sx;
        m_dim[1] = sy;
        m_dim[2] = sz;
    }
    void setExact(bool exact) { m_exact = exact; }

    size_t capacity() const { return m_capacity; }
    void reserve(const size_t new_capacity)
    {
        if (new_capacity > capacity())
            reserve_or_shrink(new_capacity);
    }
    void reserve_or_shrink(const size_t capacity)
    {
        pointer new_data = capacity > 0 ? m_allocator.allocate(capacity) : nullptr;
        const size_t n = capacity < m_size ? capacity : m_size;
        if (m_data && new_data) {
            if (std::is_trivially_copyable<T>::value) {
                ::memcpy(&*new_data, &*m_data, sizeof(T) * n);
            } else {
                for (size_t i = 0; i < n; ++i) {
                    new (&new_data[i]) T(std::move(m_data[i]));
                }
            }
        }
        if (m_data) {
            if (!std::is_trivially_copyable<T>::value) {
                for (size_t i = n; i < m_size; ++i) {
                    m_data[i].~T();
                }
            }
            m_allocator.deallocate(m_data, m_capacity);
        }
        m_data = new_data;
        m_capacity = capacity;
    }
    void shrink_to_fit()
    {
        reserve_or_shrink(m_size);
        assert(m_capacity == m_size);
    }

    bool bounds_valid() const { return m_max >= m_min; }

    void invalidate_bounds()
    {
        m_min = std::numeric_limits<value_type>::max();
        m_max = std::numeric_limits<value_type>::lowest();
    }

    void update_bounds()
    {
        invalidate_bounds();

        if (!m_data)
            return;

        for (auto it = begin(); it != end(); ++it) {
            m_min = std::max(m_min, *it);
            m_max = std::max(m_max, *it);
        }
    }

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


private:
    const uint32_t m_type;
    size_t m_size = 0;
    size_t m_dim[3] = {0, 1, 1};
    size_t m_capacity = 0;
    bool m_exact = std::is_integral<T>::value;
    value_type m_min = std::numeric_limits<value_type>::max();
    value_type m_max = std::numeric_limits<value_type>::lowest();
    pointer m_data;
    allocator m_allocator;

    ARCHIVE_ACCESS_SPLIT
    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);

    // not implemented
    shm_array(const shm_array &other) = delete;
    shm_array &operator=(const shm_array &rhs) = delete;
};

} // namespace vistle
#endif

#ifndef SHM_ARRAY_IMPL_H
#define SHM_ARRAY_IMPL_H

namespace vistle {

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::save(Archive &ar) const
{
    ar &V_NAME(ar, "type", m_type);
    ar &V_NAME(ar, "size", size_type(m_size));
    ar &V_NAME(ar, "exact", m_exact);
    ar &V_NAME(ar, "min", m_min);
    ar &V_NAME(ar, "max", m_max);
    //std::cerr << "saving array: exact=" << m_exact << ", size=" << m_size << std::endl;
    if (m_size > 0) {
        if (m_dim[0] * m_dim[1] * m_dim[2] == m_size)
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_dim[0], m_dim[1], m_dim[2]));
        else
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_size));
    }
}

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::load(Archive &ar)
{
    uint32_t type = 0;
    ar &V_NAME(ar, "type", type);
    assert(m_type == type);
    size_type size = 0;
    ar &V_NAME(ar, "size", size);
    resize(size);
    ar &V_NAME(ar, "exact", m_exact);
    ar &V_NAME(ar, "min", m_min);
    ar &V_NAME(ar, "max", m_max);
    if (m_size > 0) {
        if (m_dim[0] * m_dim[1] * m_dim[2] == m_size)
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_dim[0], m_dim[1], m_dim[2]));
        else
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_size));
    }
}

} // namespace vistle
#endif
