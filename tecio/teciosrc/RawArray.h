 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <algorithm>
#include <array>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "ClassMacros.h"
#include "CodeContract.h"
#include "MiscMacros.h"
namespace tecplot { template <typename T> class ___3267 { public: void ___2317( char const* container, size_t numElements) {
 #ifdef LARGE_ARRAY_MEMORY_LOGGING
size_t const MEMTRACK_CUTOFF = size_t(1000)*size_t(1000); if ( numElements * sizeof(T) >= MEMTRACK_CUTOFF ) { FILE *file = fopen("memtrack.txt", "at"); if ( file ) { fprintf(file, "%s\t%" "I64u" "\t%" "I64u" "\t%s\n", container, numElements, sizeof(T), typeid(T).___2683()); fclose(file); } else throw std::bad_alloc(); }
 #else
___4276(container); ___4276(numElements);
 #endif
} typedef T value_type;
 #if (defined _MSC_VER && __cplusplus > 199711L) || __cplusplus >= 201103L
___3267(___3267 const&) = delete; ___3267& operator=(___3267 const&) = delete;
 #if defined _MSC_VER && _MSC_VER <= 1800 
___3267(___3267&& ___2886) : m_valuesRef(std::move(___2886.m_valuesRef)), ___2669(std::move(___2886.___2669)), m_capacity(std::move(___2886.m_capacity)), m_size(std::move(___2886.m_size)) {} ___3267& operator=(___3267&& ___3390) { if (this != &___3390) { m_valuesRef = std::move(___3390.m_valuesRef); ___2669 = std::move(___3390.___2669); m_capacity = std::move(___3390.m_capacity); m_size = std::move(___3390.m_size); } return *this; }
 #else
___3267(___3267&&) = default; ___3267& operator=(___3267&&) = default;
 #endif
 #else
private: ___3267(___3267 const&); ___3267& operator=(___3267 const&); public:
 #endif
inline ___3267() : m_valuesRef(NULL) , ___2669(NULL) , m_capacity(0) , m_size(0) { } template <size_t N> inline ___3267( std::array<T,N>& values, size_t           size = 0) : m_valuesRef(NULL) , ___2669(values.data()) , m_capacity(values.size()) , m_size(size) { REQUIRE(VALID_REF(this->___2669)); REQUIRE(m_capacity != 0); REQUIRE(this->m_capacity >= this->m_size); } inline ___3267( std::vector<T>& values, size_t          size = 0) : m_valuesRef(NULL) , ___2669(values.data()) , m_capacity(values.size()) , m_size(size) { REQUIRE(VALID_REF(this->___2669)); REQUIRE(m_capacity != 0); REQUIRE(this->m_capacity >= this->m_size); } inline ___3267( T* const& values, size_t    capacity, size_t    size = 0) : m_valuesRef(NULL) , ___2669(const_cast<T*>(values)) , m_capacity(capacity) , m_size(size) { REQUIRE(VALID_REF(this->___2669)); REQUIRE(m_capacity != 0); REQUIRE(this->m_capacity >= this->m_size); } inline ___3267( T*&    values, size_t capacity = 0, size_t size = 0) : m_valuesRef(&values) , ___2669(values) , m_capacity(capacity) , m_size(size) { REQUIRE(VALID_REF(this->___2669) || this->___2669 == NULL); REQUIRE(EQUIVALENCE(this->___2669 == NULL, m_capacity == 0)); REQUIRE(this->m_capacity >= this->m_size); } inline void nullify() { if (m_valuesRef != NULL) *m_valuesRef = NULL; m_valuesRef = NULL; ___2669 = NULL; m_capacity = 0; m_size = 0; } inline bool empty() const { return m_size == 0; } inline size_t size() const { return m_size; } inline void reserve(size_t size) { if (size > m_capacity) enlargeCapacity(size); } inline void ___3501(size_t size) { REQUIRE(size <= m_capacity); m_size = size; ENSURE(m_size <= m_capacity); } inline void clear() { m_size = 0; } inline size_t capacity() const { return m_capacity; } inline ___3267& copy(___3267 const& ___2886) { reserve(___2886.m_size); ___3501(___2886.m_size); for (size_t ___2863 = 0; ___2863 < m_size; ++___2863) ___2669[___2863] = ___2886.___2669[___2863]; return *this; } inline ___3267& append(___3267 const& ___2886) { size_t const origSize = m_size; reserve(m_size + ___2886.m_size); ___3501(m_size + ___2886.m_size); for (size_t ___2863 = origSize; ___2863 < m_size; ++___2863) ___2669[___2863] = ___2886.___2669[___2863-origSize]; return *this; } inline void push_back(T const& value) { size_t const requestedCapacity = m_size + 1ul; if (requestedCapacity > m_capacity) { size_t const defaultCapacity = 32ul; size_t const blockSize = std::max(defaultCapacity, m_capacity / 2ul); size_t const adjustedRequest = ((requestedCapacity - 1ul) / blockSize + 1ul) * blockSize; ___476(adjustedRequest >= requestedCapacity); reserve(adjustedRequest); } size_t const valuePos = m_size; ___3501(m_size + 1ul); ___2669[valuePos] = value; } inline void copyTo(T* target) { for (size_t ___2863 = 0; ___2863 < m_size; ++___2863) target[___2863] = ___2669[___2863]; } inline T& front() { REQUIRE(this->m_size != 0 || this->m_capacity != 0); return ___2669[0]; } inline T const& front() const { REQUIRE(this->m_size != 0 || this->m_capacity != 0);
return ___2669[0]; } inline T& back() { REQUIRE(this->m_size != 0); return ___2669[m_size-1]; } inline T const& back() const { REQUIRE(this->m_size != 0); return ___2669[m_size-1]; } inline T& operator[](size_t ___2863) { REQUIRE(___2863 < this->m_size || (___2863 == 0 && this->m_capacity != 0)); return ___2669[___2863]; } inline T const& operator[](size_t ___2863) const { REQUIRE(___2863 < this->m_size || (___2863 == 0 && this->m_capacity != 0)); return ___2669[___2863]; } bool operator==(___3267<T> const& ___3390) const { bool equal = m_size == ___3390.m_size; for (size_t ___2863 = 0; equal && ___2863 < m_size; ++___2863) equal = ___2669[___2863] == ___3390.___2669[___2863]; return equal; } bool operator!=(___3267<T> const& ___3390) const { return !(*this == ___3390); } inline bool ___2033() const { return (m_valuesRef == NULL || *m_valuesRef == NULL); } inline T* data() { return ___2669; } inline T const* data() const { return ___2669; } typedef T const* const_iterator; inline const_iterator begin() const { return ___2669; } inline const_iterator end() const { return ___2669+m_size; } typedef T* iterator; inline iterator begin() { return ___2669; } inline iterator end() { return ___2669+m_size; } private: inline void enlargeCapacity(size_t capacity) { REQUIRE(capacity > m_capacity); REQUIRE(m_valuesRef != NULL); ___2317("RawArray", capacity); T* values = new T[capacity]; if (m_size != 0) {
 #if defined LINUX
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Warray-bounds"
 #pragma GCC diagnostic ignored "-Wstringop-overflow"
 #endif
std::copy(___2669, ___2669+m_size, values);
 #if defined LINUX
 #pragma GCC diagnostic pop
 #endif
} delete [] ___2669; *m_valuesRef = values; ___2669     = values; m_capacity   = capacity; } T**    m_valuesRef; T*     ___2669; size_t m_capacity; size_t m_size; }; }
