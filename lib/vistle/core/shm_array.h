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

namespace vistle {

template<typename T, class allocator>
class shm_array;

template<typename T, class allocator>
std::ostream &operator<<(std::ostream &os, const shm_array<T, allocator> &arr);

template<typename T, class allocator>
class shm_array: public ShmData {
    friend std::ostream &operator<< <T, allocator>(std::ostream &os, const shm_array<T, allocator> &arr);

public:
    typedef T value_type;
    typedef uint64_t size_type;
    typedef const value_type &const_reference;

    static unsigned typeId();

    shm_array(const allocator &alloc = allocator());
    shm_array(size_t size, const allocator &alloc = allocator());
    shm_array(size_t size, const T &value, const allocator &alloc = allocator());
    shm_array(shm_array &&other);
    ~shm_array();

    unsigned type() const { return m_type; }
    bool check() const;

    typedef typename std::allocator_traits<allocator>::pointer pointer;
    typedef T *iterator;
    typedef const T *const_iterator;

    iterator begin() const { return &*m_data; }
    iterator end() const { return (&*m_data) + m_size; }
    T *data() const { return &*m_data; }

    T &operator[](const size_t idx) { return m_data[idx]; }
    T &operator[](const size_t idx) const { return m_data[idx]; }
    T &at(const size_t idx);
    T &at(const size_t idx) const;
    void push_back(const T &v);

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
    void clear();

    size_t size() const { return m_size; }
    void resize(const size_t size);
    void resize(const size_t size, const T &value);

    void clearDimensionHint();
    void setDimensionHint(const size_t sx, const size_t sy = 1, const size_t sz = 1);
    void setExact(bool exact);

    size_t capacity() const { return m_capacity; }
    void reserve(const size_t new_capacity);
    void reserve_or_shrink(const size_t capacity);
    void shrink_to_fit();

    bool bounds_valid() const { return m_max >= m_min; }
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

#define SHMARR_EXPORT(T) \
    extern template class V_COREEXPORT shm_array<T, shm_allocator<T>>; \
    extern template V_COREEXPORT std::ostream &operator<< <T, shm_allocator<T>>( \
        std::ostream &, const shm_array<T, shm_allocator<T>> &);

FOR_ALL_SCALARS(SHMARR_EXPORT)
#if 0
SHMARR_EXPORT(CelltreeNode1)
SHMARR_EXPORT(CelltreeNode2)
SHMARR_EXPORT(CelltreeNode3)
#endif

#undef SHMARR_EXPORT

} // namespace vistle
#endif
