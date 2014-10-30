#ifndef SHM_ARRAY_H
#define SHM_ARRAY_H

#include <boost/type_traits.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include "exception.h"
#include "export.h"

namespace vistle {

#if __cplusplus < 201103L
#define nullptr 0
#endif

template<typename T, class allocator>
class shm_array {

 public: 
   typedef T value_type;
   typedef const value_type &const_reference;

   shm_array(const allocator &alloc = allocator())
   : m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {}

   shm_array(const size_t size, const allocator &alloc = allocator())
   : m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {
      resize(size);
   }

   shm_array(const size_t size, const T &value, const allocator &alloc = allocator())
   : m_size(0)
   , m_capacity(0)
   , m_data(nullptr)
   , m_allocator(alloc)
   {
      resize(size, value);
   }

#if 0
   template< class InputIt >
      shm_array( InputIt first, InputIt last, 
            const allocator &alloc = allocator() );
#endif

   ~shm_array() {
      reserve(0);
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

#if __cplusplus >= 201103L
   template <class... Args>
   void emplace_back(Args &&...args) {
      if (m_size >= m_capacity)
         reserve(m_capacity==0 ? 1 : m_capacity*2);
      assert(m_size < m_capacity);
      new(&m_data[m_size])T(std::forward<Args>(args)...);
      ++m_size;
   }
#endif

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
   void reserve(const size_t capacity) {
      pointer new_data = capacity>0 ? m_allocator.allocate(capacity) : nullptr;
      const size_t n = capacity<m_size ? capacity : m_size;
      if (m_data && new_data) {
         if (boost::has_trivial_copy<T>::value) {
            ::memcpy(&*new_data, &*m_data, sizeof(T)*n);
         } else {
            for (size_t i=0; i<n; ++i) {
#if __cplusplus >= 201103L
               new(&new_data[i])T(std::move(m_data[i]));
#else
#endif
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
   void shrink_to_fit() { reserve(m_size); assert(m_capacity == m_size); }

 private:
   size_t m_size;
   size_t m_capacity;
   pointer m_data;
   allocator m_allocator;

   friend class boost::serialization::access;
   template<class Archive>
      void serialize(Archive &ar, const unsigned int version);

   // not implemented
   shm_array(const shm_array &other);
   shm_array &operator=(const shm_array &rhs);
};

} // namespace vistle
#endif
