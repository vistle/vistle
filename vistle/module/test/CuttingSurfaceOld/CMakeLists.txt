add_module(CuttingSurfaceOld CuttingSurface.cpp)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
