#include <cstring>
#include <limits>
#include <stdexcept>

#include <vtkm/Types.h>

// We need to cast vistle floats (vistle::Scalar) to vtkm floats (vtkm::FloatDefault)
// and vice-versa. So, in this program, we check if they are compatible.
int main(int argc, char **argv)
{
    // We expect this program to be called with ${VISTLE_DOUBLE_PRECISION}.
    assert(argc == 2);
    bool usingDoublePrecision = (std::string(argv[1]) == "ON");

    auto vtkmFloatSize = sizeof(vtkm::FloatDefault);

    if ((vtkmFloatSize == sizeof(float)) && usingDoublePrecision)
        throw std::runtime_error("Vistle and vtk-m floating point types are incompatible!");

    if ((vtkmFloatSize == sizeof(double)) && !usingDoublePrecision)
        throw std::runtime_error("Vistle and vtk-m floating point types are incompatible!");

    return 0;
}
