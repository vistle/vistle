#ifndef VISTLE_CORE_INDEX_H
#define VISTLE_CORE_INDEX_H

#include <cstdlib>
#include <cinttypes>
#include <limits>
#include <type_traits>

namespace vistle {

typedef uint64_t Index64;
typedef int64_t SIndex64;
const Index64 InvalidIndex64 =
    std::numeric_limits<SIndex64>::max(); // in order to keep static_cast<Integer>(InvalidIndex) positive
//
typedef uint32_t Index32;
typedef int32_t SIndex32;
const Index32 InvalidIndex32 = ~(Index32)(0);

#ifdef VISTLE_INDEX_64BIT
typedef Index64 Index;
typedef SIndex64 SIndex;
const Index InvalidIndex = InvalidIndex64;
#else
typedef Index32 Index;
typedef SIndex32 SIndex;
const Index InvalidIndex = InvalidIndex32;
#endif

#define V_INDEX_CHECK(t) \
    static_assert(sizeof(t) == sizeof(S##t)); \
    static_assert(std::is_signed<S##t>::value); \
    static_assert(std::is_unsigned<t>::value); \
    static_assert(Invalid##t > t(0)); \
    //static_assert(static_cast<Integer>(Invalid##t) > Integer(0));

#define FOR_ALL_INDEX(MACRO) \
    MACRO(Index32) \
    MACRO(Index64)

FOR_ALL_INDEX(V_INDEX_CHECK)

} // namespace vistle

#endif
