#include <cstring>
#include <limits>
#include <stdexcept>

#include <vtkm/Types.h>


// We need to cast vistle indices (vistle::Index) to vtkm indices (vtkm::Id) and
// vice-versa. So, in this program, we check if they are compatible.
int main(int argc, char *argv[])
{
    // We expect this program to be called with one command line argument:
    // 0 (vistle is using 32-bit integers) OR 1 (vistle is using 64-bit integers)
    assert(argc == 2);
    auto vtkmLimit = std::numeric_limits<vtkm::Id>::max();

    if ((vtkmLimit == std::numeric_limits<int32_t>::max()) && (atoi(argv[1]) == 1))
        throw std::runtime_error("Vistle and vtk-m integer types are incompatible!");

    if ((vtkmLimit == std::numeric_limits<int64_t>::max()) && (atoi(argv[1]) == 0))
        throw std::runtime_error("Vistle and vtk-m integer types are incompatible!");

    return 0;
}
