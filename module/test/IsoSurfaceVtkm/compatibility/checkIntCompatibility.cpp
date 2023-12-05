#include <cstring>
#include <limits>
#include <stdexcept>

#include <vtkm/Types.h>

// We need to cast vistle indices (vistle::Index) to vtkm indices (vtkm::Id) and
// vice-versa. So, in this program, we check if they are compatible.
int main(int argc, char **argv)
{
    // We expect this program to be called with ${VISTLE_64BIT_INDICES}.
    assert(argc == 2);
    bool using64Bit = (std::string(argv[1]) == "ON");

    auto vtkmIntSize = sizeof(vtkm::Id);

    if ((vtkmIntSize == sizeof(int32_t)) && using64Bit)
        throw std::runtime_error("Vistle and vtk-m integer types are incompatible!");

    if ((vtkmIntSize == sizeof(int64_t)) && !using64Bit)
        throw std::runtime_error("Vistle and vtk-m integer types are incompatible!");

    return 0;
}
