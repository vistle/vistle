use_openmp()

include_directories(
        ../../../3rdparty/boost-mpi/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${CUDA_INCLUDE_DIRS}
        ${THRUST_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
)

add_module(IsoSurfaceOld IsoSurface.cpp)

target_link_libraries(IsoSurfaceOld
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
        ${TBB_LIBRARIES}
)

