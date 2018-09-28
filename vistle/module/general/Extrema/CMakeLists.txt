#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS mpi)

add_module(Extrema Extrema.cpp)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
