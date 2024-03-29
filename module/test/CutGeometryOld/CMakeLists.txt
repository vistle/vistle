use_openmp()
add_definitions(-DCUTGEOMETRYOLD)
add_definitions(-DCUTGEOMETRY)
add_definitions(-DCUTTINGSURFACE)
add_definitions(-DTOGGLESIGN)

#vistle_find_package(CUDA)
if(NOT APPLE
   AND CUDA_FOUND
   AND FALSE)
    include_directories(SYSTEM ${CUDA_INCLUDE_DIRS})
    add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA)
    cuda_compile(CUDA_OBJ "../../map/IsoSurface/Leveller.cu")
else()
    set(SOURCES ${SOURCES} "../../map/IsoSurface/Leveller.cpp")
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
    include_directories(SYSTEM ${TBB_INCLUDE_DIRS})
else()
    add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP)
    add_definitions(-DUSE_CPP)
endif()

include_directories(SYSTEM ${THRUST_INCLUDE_DIR})

add_module(CutGeometryOld "clip geometry at basic geometry like plane, cylinder or sphere" CutGeometry.cpp ../../map/IsoSurface/IsoDataFunctor.cpp)
