 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>
#include "ThirdPartyHeadersEnd.h"
#include "RawArray.h"
#include "CodeContract.h"
 #define ___2894(X,Y)           ((int)(((X)-1)/(Y)+1)*(Y))
static size_t const ___3477 = 8*sizeof(___3481); static size_t const ___3496 = static_cast<___3478>(1)<<(___3477-1);
 #if defined _DEBUG
 #  define USE_FUNCTIONS_FOR_SETS
 #endif
struct ___3500 { ___3491 size; ___3479 data; struct iterator { using iterator_category = std::bidirectional_iterator_tag; using difference_type = std::ptrdiff_t; using value_type = ___3491; using pointer = ___3491*; using reference = ___3491&; iterator(___3500 const* ___2098, ___3491 ___2399); iterator(iterator const&) = default; iterator& operator=(iterator const&) = default; iterator(iterator&&) = default; iterator& operator=(iterator&&) = default; ___3491 const& operator*() const; ___3491 const* operator->() const; iterator& operator++(); iterator operator++(int); iterator& operator--(); iterator operator--(int); friend bool operator==(iterator const& a, iterator const& b); friend bool operator!=(iterator const& a, iterator const& b); private: ___3500 const* m_itemSet; ___3491   m_member; }; iterator begin() const; iterator end() const; iterator cbegin() const; iterator cend() const; std::reverse_iterator<iterator> rbegin() const; std::reverse_iterator<iterator> rend() const; std::reverse_iterator<iterator> crbegin() const; std::reverse_iterator<iterator> crend() const; };
 #define ___2054(___3474) ((___3474)==NULL)
inline size_t ___3480(___3500 const* ___3474) { REQUIRE(VALID_REF(___3474)); return ___3474->size / ___3477 * sizeof(___3481); } ___3500* ___29(___372 ___3572); void ___937(___3500** ___3474); ___372 ___3494(void       *___2096, ___90  ___492); ___372 ___1199( ___3500*     ___3474, ___3491 max_val, ___372  ___3572); ___372 AllocAndCopySet( ___3500*&      ___1119, ___3500 const* ___3654); ___372 ___674( ___3500*       ___1119, ___3500 const* ___3654, ___372    ___3572); ___372 ___83( ___3500*       ___1119, ___3500 const* ___3654, ___372    ___3572); void ___491(___3500* ___3474);
 #if defined USE_FUNCTIONS_FOR_SETS
___372 ___17( ___3500*     ___3474, ___3491 ___2399, ___372  ___3572);
 #else
 #if defined __cplusplus
inline ___372 ___17( ___3500*     ___3474, ___3491 ___2399, ___372  ___3572) { if (___3474 && (___2399 + 1 <= ___3474->size || ___1199(___3474, ___2399 + 1, ___3572))) { ___3491 word = ___2399 / ___3477; ___3478 bit = (___3478)1 << (___2399 % ___3477); ___3474->data[word] |= bit; return ___4224; } else return ___1303; }
 #elif defined TECPLOTKERNEL
 #define ___17(___3474,___2399,___3572) \
 (((___3474) && \
 ((___2399)+1 <= (___3474)->size || \
 ___1199((___3474), (___2399)+1, (___3572)))) \
 ? (((___3474)->data[(___2399) / ___3477].___1344((___3478)1 << ((___2399) % ___3477))), ___4224) \
 : ___1303)
 #else
 #define ___17(___3474,___2399,___3572) \
 (((___3474) && \
 ((___2399)+1 <= (___3474)->size || \
 ___1199((___3474), (___2399)+1, (___3572)))) \
 ? (((___3474)->data[(___2399) / ___3477] |= (___3478)1 << ((___2399) % ___3477)), ___4224) \
 : ___1303)
 #endif
 #endif
___372 AddRangeToSet( ___3500*     ___3474, ___3491 rangeStart, ___3491 rangeEnd); void ___3332( ___3500*     ___3474, ___3491 ___2399); void ___955( ___3500*     ___3474, ___3491 ___2400); ___372 ___1953( ___3500*     ___3474, ___3491 ___2400, ___372  ___3569);
 #if defined USE_FUNCTIONS_FOR_SETS
___372 ___1954( ___3500 const* ___3474, ___3491   ___2399);
 #else
 #if defined __cplusplus
inline ___372 ___1954( ___3500 const* ___3474, ___3491   ___2399) { if (___3474 && (0 <= ___2399 && ___2399 < ___3474->size)) { ___3491 word = ___2399 / ___3477; ___3478 bit = (___3478)1 << (___2399 % ___3477); return (___3474->data[word]&bit) != 0; } else return ___1303; }
 #elif defined TECPLOTKERNEL
 #define ___1954(___3474,___2399)  ((___3474 && (0<=(___2399) && (___2399)<(___3474)->size)) \
 ? ((___3474)->data[(___2399)/___3477].load()&((___3478)1<<((___2399)%___3477)))!=0 \
 : ___1303)
 #else
 #define ___1954(___3474,___2399)  ((___3474 && (0<=(___2399) && (___2399)<(___3474)->size)) \
 ? ((___3474)->data[(___2399)/___3477]&((___3478)1<<((___2399)%___3477)))!=0 \
 : ___1303)
 #endif
 #endif
___372 ___2013(___3500 const* ___3474); ___372 ___1820(___3500 const* ___3474); ___3491 ___2401(___3500 const* ___3474); ___372 ___2031(___3500 const* ___3474); ___3491 ___1759( ___3500 const* ___3474, ___3491   ___3680); ___3491 ___1767( ___3500 const* ___3474, ___3491   ___3680); ___372 ___1173( ___3500 const* ___3475, ___3500 const* ___3476); ___3500* intersection( ___3500 const* ___3475, ___3500 const* ___3476); ___372 ___2060( ___3500 const* ___484, ___3500 const* ___2971); ___3491 ___2402( ___3500 const* ___3474, ___3491   ___2400); ___3491 ___2865( ___3500 const* ___3474, ___3491   ___2864); ___372 ___675( ___3500*       ___1124, ___3491   ___1123, ___3500 const* ___3661, ___3491   ___3660); void ___3558( ___3500*     ___3474, ___3491 ___3556, ___3491 ___3557, ___3491 ___3554);
 #define ___1744(___3474) (___1759((___3474), ___333))
 #define ___1749(___3474)  (___1767((___3474), ___333))
 #define ___1470(___2400, ___3474) \
 for (___2400 = ___1744((___3474)); \
 ___2400 != ___333; \
 ___2400 = ___1759((___3474), (___2400)))
 #define ForAllMembersInEntIndexSet(___2400, ___3474) \
 for (___2400 = static_cast<___1170>(___1744((___3474))); \
 ___2400 != static_cast<___1170>(___333); \
 ___2400 = static_cast<___1170>(___1759((___3474), (___2400))))
 #define ___1469(___2400, ___3474) \
 for (___2400 = ___1749((___3474)); \
 ___2400 != ___333; \
 ___2400 = ___1767((___3474), (___2400)))
inline ___3500::iterator::iterator( ___3500 const* ___2098, ___3491   ___2399) : m_itemSet{___2098} , m_member{___2399} { } inline ___3491 const& ___3500::iterator::operator*() const { return m_member; } inline ___3491 const* ___3500::iterator::operator->() const { return &m_member; } inline ___3500::iterator& ___3500::iterator::operator++() { m_member = ___1759(m_itemSet, m_member); return *this; } inline ___3500::iterator ___3500::iterator::operator++(int) { auto temp{*this}; ++(*this); return temp; } inline ___3500::iterator& ___3500::iterator::operator--() { m_member = ___1767(m_itemSet, m_member); return *this; } inline ___3500::iterator ___3500::iterator::operator--(int) { auto temp{*this}; --(*this); return temp; } inline bool operator==(___3500::iterator const& a, ___3500::iterator const& b) { return a.m_itemSet == b.m_itemSet && a.m_member == b.m_member; } inline bool operator!=(___3500::iterator const& a, ___3500::iterator const& b) { return a.m_itemSet != b.m_itemSet || a.m_member != b.m_member; } inline ___3500::iterator ___3500::begin() const { return ___3500::iterator{this, ___1759(this, ___333)}; } inline ___3500::iterator ___3500::cbegin() const { return begin(); } inline ___3500::iterator ___3500::end() const { return ___3500::iterator{this, ___333}; } inline ___3500::iterator ___3500::cend() const { return end(); } inline std::reverse_iterator<___3500::iterator> ___3500::rbegin() const { return std::reverse_iterator<___3500::iterator>(end()); } inline std::reverse_iterator<___3500::iterator> ___3500::rend() const { return std::reverse_iterator<___3500::iterator>(begin()); } inline std::reverse_iterator<___3500::iterator> ___3500::crbegin() const { return rbegin(); } inline std::reverse_iterator<___3500::iterator> ___3500::crend() const { return rend(); } namespace tecplot { template <typename T> std::vector<T> ___4192(___3500 const* ___2098) { REQUIRE(VALID_REF(___2098) || ___2098 == 0); std::vector<T> ___3356; size_t const count = ___2401(___2098); if (count != 0) { ___3356.reserve(count); ___3491 ___2083; ___1470(___2083,___2098) ___3356.push_back(static_cast<T>(___2083)); } return ___3356; } template <typename T> inline std::set<T> ___4184(___3500 const* set) { REQUIRE(VALID_REF_OR_NULL(set)); ___1170 value; std::set<T> ___3356; if (set != NULL) { ___1470(value, set) { ___3356.insert(static_cast<T>(value)); } } return ___3356; } template <typename CONTAINER> ___3500* ___4184( CONTAINER const& ___2097, bool             isSorted = true) { REQUIRE(IMPLICATION(isSorted && !___2097.empty(), ___2097[___2097.size()-1] == ___333 || ___2097[___2097.size()-1] == *std::max_element(&___2097[0], &___2097[0]+___2097.size()))); ___3500* ___3356 = ___29(___1303); if (___3356 == NULL) throw std::bad_alloc(); if (!___2097.empty()) { typename CONTAINER::value_type largestMember = static_cast<typename CONTAINER::value_type>(___333); if (isSorted) { for (typename CONTAINER::value_type const* iter = &___2097[___2097.size()-1]; iter >= &___2097[0]; --iter) if ((largestMember = *iter) != static_cast<typename CONTAINER::value_type>(___333))
break; } else { largestMember = *std::max_element(&___2097[0], &___2097[0]+___2097.size()); } if (largestMember != static_cast<typename CONTAINER::value_type>(___333)) { if (!___1199(___3356, static_cast<___3491>(largestMember + 1), ___1303)) { ___937(&___3356); throw std::bad_alloc(); } typename CONTAINER::value_type const* itemsArray = &___2097[0]; size_t const ___2810 = ___2097.size(); for (size_t ___1990 = 0; ___1990 < ___2810; ++___1990) if (itemsArray[___1990] != static_cast<typename CONTAINER::value_type>(___333)) (void)___17(___3356,static_cast<___3491>(itemsArray[___1990]),___1303); } } ENSURE(VALID_REF(___3356)); return ___3356; } template <typename T> void ___4183( ___3500 const* ___2098, ___3267<T>& ___3356) { REQUIRE(VALID_REF(___2098) || ___2098 == 0); size_t const count = ___2401(___2098); if (count != 0) { ___3356.reserve(count); ___3356.___3501(count); T* ___3358 = &___3356[0]; size_t ___2863 = 0; ___3491 ___2083; ___1470(___2083,___2098) ___3358[___2863++] = static_cast<T>(___2083); } else { ___3356.___3501(0); } } }
