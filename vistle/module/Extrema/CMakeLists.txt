#find_package(Boost 1.40 REQUIRED COMPONENTS mpi)

add_module(Extrema Extrema.cpp)

use_openmp()

include_directories(
        ../../../3rdparty/boost-mpi/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Extrema
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
