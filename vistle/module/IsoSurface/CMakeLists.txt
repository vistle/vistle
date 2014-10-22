find_package(TBB)
use_openmp()

if(OPENMP_FOUND)
   add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP)
   add_definitions(-DUSE_OMP)
elseif(TBB_FOUND)
   add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB)
   add_definitions(-DUSE_TBB)
else()
   add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP)
   add_definitions(-DUSE_CPP)
endif()

find_package(CUDA)
if(CUDA_FOUND)
   add_definitions(-DHAVE_CUDA)
endif()


include_directories(
        ../../../3rdparty/boost-mpi/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${CUDA_INCLUDE_DIRS}
        ${THRUST_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
)

add_module(IsoSurface IsoSurface.cpp)

target_link_libraries(IsoSurface
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
        ${TBB_LIBRARIES}
)

