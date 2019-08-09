#ifndef VISTLE_UTIL_BUFFER_H
#define VISTLE_UTIL_BUFFER_H

#include "allocator.h"
#include <vector>

namespace vistle {

using buffer = std::vector<char, default_init_allocator<char>>;

}
#endif
