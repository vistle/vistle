#ifndef INDEX_H
#define INDEX_H

#include <cstdlib>
#include <cinttypes>
#include <limits>

namespace vistle {

#ifdef VISTLE_INDEX_64BIT
typedef uint64_t Index;
typedef int64_t SIndex;
const Index InvalidIndex =
    std::numeric_limits<SIndex>::max(); // in order to keep static_cast<Integer>(InvalidIndex) positive
#else
typedef uint32_t Index;
typedef int32_t SIndex;
const Index InvalidIndex = ~(Index)(0);
#endif


} // namespace vistle

#endif
