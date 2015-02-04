vistle_find_package(TBB)
use_openmp()

set(SOURCES "../IsoSurface/IsoSurface.cpp")
set(CUDA_OBJ "")
#vistle_find_package(CUDA)
if(NOT APPLE AND CUDA_FOUND)
   add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA)
   cuda_compile(CUDA_OBJ "../IsoSurface/Leveller.cu")
else()
   set(SOURCES ${SOURCES} "../IsoSurface/Leveller.cpp")
   if(OPENMP_FOUND)
      add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP)
   elseif(TBB_FOUND)
      add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB)
   else()
      add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP)
   endif()
endif()

if(OPENMP_FOUND)
   add_definitions(-DUSE_OMP)
   add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP)
elseif(TBB_FOUND)
   add_definitions(-DUSE_TBB)
   add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_TBB)
else()
   add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP)
   add_definitions(-DUSE_CPP)
endif()

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${CUDA_INCLUDE_DIRS}
        ${THRUST_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
)

add_module(${NAME} ${SOURCES} ${CUDA_OBJ})

target_link_libraries(${NAME}
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
        ${TBB_LIBRARIES}
        ${CUDA_LIBRARIES}
)
