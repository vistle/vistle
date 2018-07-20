#ifndef INDEX_H
#define INDEX_H

#include <cstdlib>
#include <cinttypes>

namespace vistle {

typedef uint32_t Index;
typedef int32_t SIndex;

const Index InvalidIndex = ~(Index)(0);

} // namespace vistle

#endif
