use_openmp()

set(SOURCES
    ${SOURCES}
    ../IsoSurface/IsoSurface.cpp
    ../IsoSurface/IsoSurface.h
    ../IsoSurface/IsoDataFunctor.cpp
    ../IsoSurface/IsoDataFunctor.h
    ../IsoSurface/Leveller.cpp
    ../IsoSurface/Leveller.h)

add_module(${NAME} ${DESCRIPTION} ${SOURCES})
target_include_directories(${NAME} SYSTEM BEFORE PRIVATE ${THRUST_INCLUDE_DIR})
target_compile_definitions(${NAME} PRIVATE -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP)

if(OPENMP_FOUND AND VISTLE_USE_OPENMP)
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP)
elseif(TBB_FOUND)
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_TBB)
    target_include_directories(${NAME} SYSTEM BEFORE PRIVATE ${TBB_INCLUDE_DIRS})
    if(${CMAKE_GENERATOR} STREQUAL "Visual Studio 17 2022")
        target_link_libraries(${NAME} ${TBB_LIBRARIES})
    else()
        target_link_libraries(${NAME} TBB::tbb)
    endif()
else()
    target_compile_definitions(${NAME} PRIVATE -DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP)
endif()
