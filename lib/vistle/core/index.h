#ifndef VISTLE_CORE_INDEX_H
#define VISTLE_CORE_INDEX_H

#include <cstdlib>
#include <cinttypes>
#include <limits>

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


} // namespace vistle

#endif
