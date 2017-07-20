vistle_find_package(TBB)
use_openmp()
add_definitions(-DCUTGEOMETRY)
add_definitions(-DCUTTINGSURFACE)
add_definitions(-DTOGGLESIGN)

#vistle_find_package(CUDA)
if(NOT APPLE AND CUDA_FOUND AND FALSE)
   include_directories(
        ${CUDA_INCLUDE_DIRS}
   )
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
        ${THRUST_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
)

add_module(CutGeometry CutGeometry.cpp ../IsoSurface/IsoDataFunctor.cpp)

target_link_libraries(CutGeometry
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
