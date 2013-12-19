
use_openmp()

add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP)

#find_package(CUDA)
#if(CUDA_FOUND)
##   add_definitions(-DHAVE_CUDA)
#endif()


include_directories(
        ../../../3rdparty/boost-mpi/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${CUDA_INCLUDE_DIRS}
        ${THRUST_INCLUDE_DIR}
)

add_module(IsoSurface IsoSurface.cpp)

target_link_libraries(IsoSurface
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
