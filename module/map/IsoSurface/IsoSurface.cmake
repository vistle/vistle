set(SOURCES
    ${SOURCES}
    ../IsoSurface/IsoSurface.cpp
    ../IsoSurface/IsoSurface.h
    ../IsoSurface/IsoDataFunctor.cpp
    ../IsoSurface/IsoDataFunctor.h
    ../IsoSurface/Leveller.cpp
    ../IsoSurface/Leveller.h)

add_module(${NAME} ${DESCRIPTION} ${SOURCES})

target_include_directories(${NAME} BEFORE PRIVATE ../IsoSurface/cccl/thrust)
target_include_directories(${NAME} BEFORE PRIVATE ../IsoSurface/cccl/cub)
target_include_directories(${NAME} BEFORE PRIVATE ../IsoSurface/cccl/libcudacxx/include)

target_compile_definitions(${NAME} PRIVATE -DCCCL_THRUST_DEVICE_SYSTEM=CPP)
target_compile_definitions(${NAME} PRIVATE -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP)

if(OPENMP_FOUND AND VISTLE_USE_OPENMP)
    target_compile_definitions(${NAME} PRIVATE -DCCCL_THRUST_HOST_SYSTEM=OMP)
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP)
    target_link_libraries(${NAME} PRIVATE OpenMP::OpenMP_CXX)
elseif(TBB_FOUND)
    target_include_directories(${NAME} SYSTEM BEFORE PRIVATE ${TBB_INCLUDE_DIRS})
    target_compile_definitions(${NAME} PRIVATE -DCCCL_THRUST_HOST_SYSTEM=TBB)
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_TBB)
    if(${CMAKE_GENERATOR} STREQUAL "Visual Studio 17 2022")
        target_link_libraries(${NAME} PRIVATE ${TBB_LIBRARIES})
    else()
        target_link_libraries(${NAME} PRIVATE TBB::tbb)
    endif()
else()
    target_compile_definitions(${NAME} PRIVATE -DCCCL_THRUST_HOST_SYSTEM=CPP)
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP)
endif()
