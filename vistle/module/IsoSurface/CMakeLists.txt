add_executable(IsoSurface IsoSurface.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ../../../3rdparty/boost-mpi/include
)

target_link_libraries(IsoSurface
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
