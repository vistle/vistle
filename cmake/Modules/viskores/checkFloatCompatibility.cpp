#include <cstring>
#include <limits>
#include <stdexcept>

#include <viskores/Types.h>

// We need to cast vistle floats (vistle::Scalar) to viskores floats (viskores::FloatDefault)
// and vice-versa. So, in this program, we check if they are compatible.
int main(int argc, char **argv)
{
    // We expect this program to be called with ${VISTLE_DOUBLE_PRECISION}.
    assert(argc == 2);
    bool usingDoublePrecision = (std::string(argv[1]) == "ON");

    auto viskoresFloatSize = sizeof(viskores::FloatDefault);

    if ((viskoresFloatSize == sizeof(float)) && usingDoublePrecision)
        throw std::runtime_error("Vistle and Viskores floating point types are incompatible!");

    if ((viskoresFloatSize == sizeof(double)) && !usingDoublePrecision)
        throw std::runtime_error("Vistle and Viskores floating point types are incompatible!");

    return 0;
}
