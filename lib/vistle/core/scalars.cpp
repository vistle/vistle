#include "scalars.h"

namespace vistle {

const char *scalarTypeName(unsigned id)
{
    if (id >= ScalarTypeNames.size()) {
        return "unknown";
    }
    return ScalarTypeNames[id];
}

} // namespace vistle
