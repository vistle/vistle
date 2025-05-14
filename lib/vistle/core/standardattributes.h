#ifndef VISTLE_CORE_STANDARDATTRIBUTES_H
#define VISTLE_CORE_STANDARDATTRIBUTES_H

namespace vistle {

namespace attribute {
// clang-format off
#define A(v, n) constexpr auto v{#n}
// clang-format on

#include "standardattributelist.h"

#undef A
}; // namespace attribute

} // namespace vistle
#endif
