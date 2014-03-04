add_module(IsoSurface IsoSurface.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ../../../3rdparty/boost-mpi/include
)

target_link_libraries(IsoSurface
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
