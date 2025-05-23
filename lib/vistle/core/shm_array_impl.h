#ifndef VISTLE_CORE_SHM_ARRAY_IMPL_H
#define VISTLE_CORE_SHM_ARRAY_IMPL_H

#include <cassert>
#include <atomic>
#include <cstring>
#include <type_traits>
#include <iostream>

#include <vistle/util/exception.h>
#include <vistle/util/tools.h>
#include "export.h"
#include "index.h"
#include "archives_config.h"
#include "shmdata.h"
#include <viskores/cont/ArrayRangeCompute.h>
#include <boost/mpl/for_each.hpp>

namespace vistle {

template<typename T, class allocator>
shm_array<T, allocator>::shm_array(const allocator &alloc)
: ShmData(ShmData::ARRAY)
, m_type(typeId())
, m_size(0)
, m_capacity(0)
, m_data(nullptr)
#ifndef NO_SHMEM
, m_allocator(alloc)
#endif
{
    resize(0);
}

template<typename T, class allocator>
shm_array<T, allocator>::shm_array(const size_t size, const allocator &alloc)
: ShmData(ShmData::ARRAY)
, m_type(typeId())
, m_size(0)
, m_capacity(0)
, m_data(nullptr)
#ifndef NO_SHMEM
, m_allocator(alloc)
#endif
{
    resize(size);
}

template<typename T, class allocator>
shm_array<T, allocator>::shm_array(const size_t size, const T &value, const allocator &alloc)
: ShmData(ShmData::ARRAY)
, m_type(typeId())
, m_size(0)
, m_capacity(0)
, m_data(nullptr)
#ifndef NO_SHMEM
, m_allocator(alloc)
#endif
{
    resize(size, value);
}

template<typename T, class allocator>
shm_array<T, allocator>::shm_array(shm_array &&other)
: ShmData(ShmData::ARRAY)
, m_type(other.m_type)
, m_size(other.m_size)
, m_capacity(other.m_capacity)
, m_min(other.m_min)
, m_max(other.m_max)
, m_data(other.m_data)
#ifdef NO_SHMEM
, m_memoryValid(other.m_memoryValid.load(std::memory_order_seq_cst))
, m_unknown(other.m_unknown)
, m_handle(other.m_handle)
#else
, m_allocator(other.m_allocator)
#endif
{
    other.m_data = nullptr;
    other.m_size = 0;
    other.m_capacity = 0;
    other.invalidate_bounds();
    assert(typeId() == m_type);
}

template<typename T, class allocator>
shm_array<T, allocator>::~shm_array()
{
    assert(refcount() == 0);
    reserve_or_shrink(0);
}

template<typename T, class allocator>
template<class ArrayHandle>
void shm_array<T, allocator>::setHandle(const ArrayHandle &h)
{
    //assert(h.GetValueTypeName() == viskores::cont::TypeToString(handle_type()));
#ifdef NO_SHMEM
    PROF_SCOPE("shm_array::setHandle()");
    m_unknown = h;
    m_memoryValid = false;
    m_size = h.GetNumberOfValues();
    if (m_unknown.CanConvert<viskores::cont::ArrayHandleBasic<handle_type>>()) {
        m_handle = m_unknown.AsArrayHandle<viskores::cont::ArrayHandleBasic<handle_type>>();
    } else {
        viskores::cont::ArrayCopy(m_unknown, m_handle);
    }
    m_capacity = m_size;

    viskores::cont::ArrayHandle<viskores::Range> rangeArray = viskores::cont::ArrayRangeCompute(h);
    auto rangePortal = rangeArray.ReadPortal();
    assert(rangePortal.GetNumberOfValues() == 1); // 1 component
    viskores::Range componentRange = rangePortal.Get(0);
    m_min = componentRange.Min;
    m_max = componentRange.Max;
    assert(m_size == 0 || bounds_valid());
#else
    resize(h.GetNumberOfValues());
    viskores::cont::ArrayHandleBasic<handle_type> handle(reinterpret_cast<handle_type *>(m_data.get()), m_size,
                                                         [](void *) {});
    viskores::cont::ArrayCopy(h, handle);
    handle.SyncControlArray();
    invalidate_bounds();
#endif
}

template<typename T, class allocator>
bool shm_array<T, allocator>::check(std::ostream &os) const
{
    assert(refcount() >= 0);
#ifndef NO_SHMEM
    assert(m_size <= m_capacity);
#endif
    if (refcount() < 0) {
        os << "shm_array: INCONSISTENCY: refcount() < 0" << std::endl;
        return false;
    }
#ifndef NO_SHMEM
    if (m_size > m_capacity) {
        os << "shm_array: INCONSISTENCY: m_size > m_capacity" << std::endl;
        return false;
    }
#endif
    if (m_dim[0] != 0 && m_size != m_dim[0] * m_dim[1] * m_dim[2]) {
        os << "shm_array: INCONSISTENCY: dimensions" << std::endl;
        return false;
    }
    return true;
}

template<typename T, class allocator>
T &shm_array<T, allocator>::at(const size_t idx)
{
    updateFromHandle(true);
    if (idx >= m_size)
        throw(std::out_of_range("shm_array"));
    return m_data[idx];
}

template<typename T, class allocator>
const T &shm_array<T, allocator>::at(const size_t idx) const
{
    updateFromHandle();
    if (idx >= m_size)
        throw(std::out_of_range("shm_array"));
    return m_data[idx];
}

template<typename T, class allocator>
void shm_array<T, allocator>::push_back(const T &v)
{
    updateFromHandle(true);
    if (m_size >= m_capacity)
        reserve(m_capacity == 0 ? 1 : m_capacity * 2);
    assert(m_size < m_capacity);
    new (&m_data[m_size]) T(v);
    ++m_size;
}

template<typename T, class allocator>
void shm_array<T, allocator>::clear()
{
    resize(0);
}

template<typename T, class allocator>
void shm_array<T, allocator>::resize(const size_t size)
{
    if (size == m_size)
        return;

    updateFromHandle(true);
    reserve(size);
    if (!std::is_trivially_copyable<T>::value) {
        for (size_t i = m_size; i < size; ++i)
            new (&m_data[i]) T();
    }
    m_size = size;
    clearDimensionHint();
}

template<typename T, class allocator>
void shm_array<T, allocator>::resize(const size_t size, const T &value)
{
    if (size == m_size)
        return;

    updateFromHandle(true);
    reserve(size);
    for (size_t i = m_size; i < size; ++i)
        new (&m_data[i]) T(value);
    m_size = size;
    clearDimensionHint();
}

#ifdef NO_SHMEM
template<typename T, class allocator>
const viskores::cont::ArrayHandle<typename shm_array<T, allocator>::handle_type> &
shm_array<T, allocator>::handle() const
{
    if (m_size != m_capacity) {
        // many Viskores algorithms check that array sizes are exact
        m_handle.Allocate(m_size, viskores::CopyFlag::On);
        m_capacity = m_size;
        if (m_memoryValid)
            m_data = reinterpret_cast<T *>(m_handle.GetWritePointer());
        else
            m_data = nullptr;
    }

    return m_handle;
}
#else
template<typename T, class allocator>
const viskores::cont::ArrayHandle<typename shm_array<T, allocator>::handle_type> shm_array<T, allocator>::handle() const
{
    return viskores::cont::make_ArrayHandle(reinterpret_cast<const handle_type *>(m_data.get()), m_size,
                                            viskores::CopyFlag::Off);
}
#endif


template<typename T, class allocator>
void shm_array<T, allocator>::clearDimensionHint()
{
    m_dim[0] = 0;
    m_dim[1] = m_dim[2] = 1;
}

template<typename T, class allocator>
void shm_array<T, allocator>::setDimensionHint(const size_t sx, const size_t sy, const size_t sz)
{
    assert(m_size == sx * sy * sz);
    m_dim[0] = sx;
    m_dim[1] = sy;
    m_dim[2] = sz;
}

template<typename T, class allocator>
void shm_array<T, allocator>::setExact(bool exact)
{
    m_exact = exact;
}

template<typename T, class allocator>
bool shm_array<T, allocator>::exact() const
{
    return m_exact;
}

template<typename T, class allocator>
size_t shm_array<T, allocator>::dimensionHint(int d) const
{
    return m_dim[d];
}

template<typename T, class allocator>
void shm_array<T, allocator>::reserve(const size_t new_capacity)
{
    if (new_capacity >= std::numeric_limits<Index>::max()) {
        std::cerr << "shm_array: cannot allocate more than " << std::numeric_limits<Index>::max() - 1 << " elements"
                  << std::endl;
        std::cerr << "           recompile with -DVISTLE_INDEX_64BIT" << std::endl;
        throw vistle::except::index_overflow("shm_array: " + std::to_string(new_capacity) +
                                             " >= " + std::to_string(std::numeric_limits<Index>::max()));
    }

    if (new_capacity >= std::numeric_limits<viskores::Id>::max()) {
        std::cerr << "shm_array: size " << new_capacity << " exceeds viskores::Id's range" << std::endl;
        std::cerr << "           recompile with -DVISTLE_INDEX_64BIT" << std::endl;
    }

    if (new_capacity > capacity())
        reserve_or_shrink(new_capacity);
}

template<typename T, class allocator>
void shm_array<T, allocator>::reserve_or_shrink(const size_t capacity)
{
    if (m_capacity == capacity)
        return;

    PROF_SCOPE("shm_array::reserve_or_shrink()");
#ifdef NO_SHMEM
    m_capacity = capacity;
    updateFromHandle(true);
    if (capacity > 0) {
        m_handle.Allocate(capacity, viskores::CopyFlag::On);
        m_data = reinterpret_cast<T *>(m_handle.GetWritePointer());
    } else {
        m_handle = viskores::cont::make_ArrayHandle(static_cast<handle_type *>(nullptr), 0, viskores::CopyFlag::Off);
        m_data = nullptr;
    }
#else
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
#endif
}

template<typename T, class allocator>
void shm_array<T, allocator>::shrink_to_fit()
{
    reserve_or_shrink(m_size);
    assert(m_capacity == m_size);
}

template<typename T, class allocator>
void shm_array<T, allocator>::invalidate_bounds()
{
    m_min = std::numeric_limits<value_type>::max();
    m_max = std::numeric_limits<value_type>::lowest();
}

template<typename T, class allocator>
void shm_array<T, allocator>::update_bounds()
{
    if (bounds_valid())
        return;

    PROF_SCOPE("shm_array::update_bounds()");
    updateFromHandle();
    invalidate_bounds();

    if (!m_data)
        return;

    for (auto it = begin(); it != end(); ++it) {
        m_min = std::min(m_min, *it);
        m_max = std::max(m_max, *it);
    }
}

template<typename T, class allocator>
void shm_array<T, allocator>::set_bounds(T min, T max)
{
    m_min = min;
    m_max = max;
}

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::save(Archive &ar) const
{
    updateFromHandle();
    ar &V_NAME(ar, "type", m_type);
    ar &V_NAME(ar, "size", size_type(m_size));
    ar &V_NAME(ar, "exact", m_exact);
    //std::cerr << "saving array: exact=" << m_exact << ", size=" << m_size << std::endl;
    if (m_size > 0) {
        if (m_dim[0] * m_dim[1] * m_dim[2] == m_size)
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_dim[0], m_dim[1], m_dim[2]));
        else
            ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_size));
    }
    ar &V_NAME(ar, "min", m_min);
    ar &V_NAME(ar, "max", m_max);
}

template<class Archive, class Array>
struct load_compat {
    Archive &m_archive;
    Array &m_array;
    typedef typename Array::value_type T;

    int m_type = -1;
    T m_min, m_max;
    bool m_ok = false;

    load_compat(Archive &ar, Array &arr, int type): m_archive(ar), m_array(arr), m_type(type) {}

    template<typename S>
    void operator()(S dummy)
    {
        if (m_type != static_cast<int>(vistle::scalarTypeId<S>()))
            return;

        if (m_array.size() > 0) {
            std::vector<S> vec(m_array.size());
            size_t d[3];
            for (int i = 0; i < 3; ++i)
                d[i] = m_array.dimensionHint(i);
            if (d[0] * d[1] * d[2] == m_array.size())
                m_archive &V_NAME(ar, "elements",
                                  detail::wrap_array<Archive>(vec.data(), m_array.exact(), d[0], d[1], d[2]));
            else
                m_archive &V_NAME(ar, "elements",
                                  detail::wrap_array<Archive>(vec.data(), m_array.exact(), m_array.size()));
            std::copy(vec.begin(), vec.end(), m_array.begin());
        }

        S min, max;
        m_archive &V_NAME(ar, "min", min);
        m_archive &V_NAME(ar, "max", max);
        m_array.set_bounds(min, max);
        m_ok = true;
    }

    bool load()
    {
        boost::mpl::for_each<typename can_load<T>::list>(boost::reference_wrapper<load_compat>(*this));
        return m_ok;
    }
};

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::load(Archive &ar)
{
    uint32_t type = 0;
    ar &V_NAME(ar, "type", type);
    size_type size = 0;
    ar &V_NAME(ar, "size", size);
    resize(size);
    ar &V_NAME(ar, "exact", m_exact);
    if (m_type == type) {
        if (m_size > 0) {
            if (m_dim[0] * m_dim[1] * m_dim[2] == m_size)
                ar &V_NAME(ar, "elements",
                           detail::wrap_array<Archive>(&m_data[0], m_exact, m_dim[0], m_dim[1], m_dim[2]));
            else
                ar &V_NAME(ar, "elements", detail::wrap_array<Archive>(&m_data[0], m_exact, m_size));
        }
        ar &V_NAME(ar, "min", m_min);
        ar &V_NAME(ar, "max", m_max);
    } else {
        load_compat<Archive, shm_array<T, allocator>> loader(ar, *this, type);
        if (!loader.load()) {
            std::cerr << "failed to restore from compatible data type" << std::endl;
        }
    }
}

template<typename T, class allocator>
void shm_array<T, allocator>::print(std::ostream &os, bool verbose) const
{
    //os << name << " ";
    os << scalarTypeName(typeId()) << "[" << size();
    if (verbose) {
        os << ":";
        for (size_t i = 0; i < size(); ++i) {
            os << " " << m_data[i];
        }
    }
    os << "]";
    os << " #ref:" << refcount();
}

template<typename T, class allocator>
std::ostream &operator<<(std::ostream &os, const shm_array<T, allocator> &arr)
{
    arr.print(os);
    return os;
}

} // namespace vistle
#endif
