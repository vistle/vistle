use_openmp()

set(PREFER_TBB TRUE)
set(USE_TBB FALSE)

set(SOURCES
    ${SOURCES}
    ../IsoSurface/IsoSurface.cpp
    ../IsoSurface/IsoSurface.h
    ../IsoSurface/IsoDataFunctor.cpp
    ../IsoSurface/IsoDataFunctor.h
    ../IsoSurface/Leveller.cpp
    ../IsoSurface/Leveller.h)

if(TBB_FOUND AND PREFER_TBB)
    add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB)
    set(USE_TBB TRUE)
elseif(OPENMP_FOUND)
    add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP)
elseif(TBB_FOUND)
    add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB)
    set(USE_TBB TRUE)
else()
    add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP)
endif()

if(TBB_FOUND AND PREFER_TBB)
    add_definitions(-DUSE_TBB)
    add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_TBB)
    set(USE_TBB TRUE)
elseif(OPENMP_FOUND)
    add_definitions(-DUSE_OMP)
    add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_OMP)
    use_openmp()
elseif(TBB_FOUND)
    add_definitions(-DUSE_TBB)
    add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_TBB)
    set(USE_TBB TRUE)
else()
    add_definitions(-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP)
    add_definitions(-DUSE_CPP)
endif()

add_module(${NAME} ${DESCRIPTION} ${SOURCES})
target_include_directories(${NAME} SYSTEM BEFORE PRIVATE ${THRUST_INCLUDE_DIR})

if(USE_TBB)
    target_include_directories(${NAME} SYSTEM BEFORE PRIVATE ${TBB_INCLUDE_DIRS})
    if(${CMAKE_GENERATOR} STREQUAL "Visual Studio 17 2022")
        target_link_libraries(${NAME} ${TBB_LIBRARIES})
    else()
        target_link_libraries(${NAME} TBB::tbb)
    endif()
endif()
