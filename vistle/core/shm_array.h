#ifndef SHM_ARRAY_H
#define SHM_ARRAY_H

#include <cassert>
#include <atomic>
#include <cstring>

#include <boost/type_traits.hpp>
#include <boost/version.hpp>
#include <boost/serialization/access.hpp>
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#else
#include <boost/serialization/array.hpp>
#endif

#include <util/exception.h>
#include "export.h"

namespace vistle {

template<typename T, class allocator>
class shm_array {

 public: 
   typedef T value_type;
   typedef const value_type &const_reference;

   static int typeId();

   shm_array(const allocator &alloc = allocator())
   : m_type(typeId())
   , m_refcount(0)
   , m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {
       resize(0);
   }

   shm_array(const size_t size, const allocator &alloc = allocator())
   : m_type(typeId())
   , m_refcount(0)
   , m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {
      resize(size);
   }

   shm_array(const size_t size, const T &value, const allocator &alloc = allocator())
   : m_type(typeId())
   , m_refcount(0)
   , m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {
      resize(size, value);
   }

   shm_array(shm_array &&other)
   : m_type(other.m_type)
   , m_refcount(0)
   , m_size(other.m_size)
   , m_capacity(other.m_capacity)
   , m_data(other.m_data)
   , m_allocator(other.m_allocator)
   {
       other.m_data = nullptr;
       other.m_size = 0;
       other.m_capacity = 0;
       assert(typeId() == m_type);
   }

   ~shm_array() {
      reserve_or_shrink(0);
   }

   int type() const { return m_type; }
   bool check() const {
       assert(m_refcount >= 0);
       assert(m_size <= m_capacity);
       if (m_refcount < 0)
           return false;
       if (m_size > m_capacity)
           return false;
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
   T &at(const size_t idx) { if (idx >= m_size) throw(std::out_of_range("shm_array")); return m_data[idx]; }
   T &at(const size_t idx) const { if (idx >= m_size) throw(std::out_of_range("shm_array")); return m_data[idx]; }

   void push_back(const T &v) {
      if (m_size >= m_capacity)
         reserve(m_capacity==0 ? 1 : m_capacity*2);
      assert(m_size < m_capacity);
      new(&m_data[m_size])T(v);
      ++m_size;
   }

   template <class... Args>
   void emplace_back(Args &&...args) {
      if (m_size >= m_capacity)
         reserve(m_capacity==0 ? 1 : m_capacity*2);
      assert(m_size < m_capacity);
      new(&m_data[m_size])T(std::forward<Args>(args)...);
      ++m_size;
   }

   T &back() { return m_data[m_size-1]; }
   T &front() { return m_data[0]; }

   bool empty() const { return m_size == 0; }
   void clear() { resize(0); }

   size_t size() const { return m_size; }
   void resize(const size_t size) {
      reserve(size);
      if (!boost::has_trivial_copy<T>::value) {
         for (size_t i=m_size; i<size; ++i)
            new(&m_data[i])T();
      }
      m_size = size;
      assert(m_size <= m_capacity);
   }
   void resize(const size_t size, const T &value) {
      reserve(size);
      for (size_t i=m_size; i<size; ++i)
         new(&m_data[i])T(value);
      m_size = size;
      assert(m_size <= m_capacity);
   }

   size_t capacity() const { return m_capacity; }
   void reserve(const size_t new_capacity) {
       if (new_capacity > capacity())
           reserve_or_shrink(new_capacity);
   }
   void reserve_or_shrink(const size_t capacity) {
      pointer new_data = capacity>0 ? m_allocator.allocate(capacity) : nullptr;
      const size_t n = capacity<m_size ? capacity : m_size;
      if (m_data && new_data) {
         if (boost::has_trivial_copy<T>::value) {
            ::memcpy(&*new_data, &*m_data, sizeof(T)*n);
         } else {
            for (size_t i=0; i<n; ++i) {
               new(&new_data[i])T(std::move(m_data[i]));
            }
         }
      }
      if (m_data) {
         if (!boost::has_trivial_copy<T>::value) {
            for (size_t i=n; i<m_size; ++i) {
               m_data[i].~T();
            }
         }
         m_allocator.deallocate(m_data, m_capacity);
      }
      m_data = new_data;
      m_capacity = capacity;
   }
   void shrink_to_fit() { reserve_or_shrink(m_size); assert(m_capacity == m_size); }

   int ref() const { return ++m_refcount; }
   int unref() const { return --m_refcount; }
   int refcount() const { return m_refcount; }

 private:
   const int m_type;
   mutable std::atomic<int> m_refcount;
   size_t m_size;
   size_t m_capacity;
   pointer m_data;
   allocator m_allocator;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive &ar, const unsigned int version);
   template<class Archive>
   void save(Archive &ar, const unsigned int version) const;
   template<class Archive>
   void load(Archive &ar, const unsigned int version);

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
void shm_array<T, allocator>::serialize(Archive &ar, const unsigned int version) {
    boost::serialization::split_member(ar, *this, version);
}

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::save(Archive &ar, const unsigned int version) const {
    ar & m_type;
    ar & m_size;
    if (m_size > 0)
       ar & boost::serialization::make_nvp("elements", boost::serialization::make_array(&m_data[0], m_size));
}

template<typename T, class allocator>
template<class Archive>
void shm_array<T, allocator>::load(Archive &ar, const unsigned int version) {
    int type = 0;
    ar & type;
    assert(m_type == type);
    size_t size = 0;
    ar & size;
    resize(size);
    if (m_size > 0)
       ar & boost::serialization::make_nvp("elements", boost::serialization::make_array(&m_data[0], m_size));
}

} // namespace vistle
#endif
