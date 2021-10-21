#ifndef INDEX_H
#define INDEX_H

#include <cstdlib>
#include <cinttypes>

namespace vistle {

#ifdef VISTLE_INDEX_64BIT
typedef uint64_t Index;
typedef int64_t SIndex;
#else
typedef uint32_t Index;
typedef int32_t SIndex;
#endif

const Index InvalidIndex = ~(Index)(0);

} // namespace vistle

#endif
