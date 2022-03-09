vistle_find_package(COVER)
if(NOT COVER_FOUND)
    message("COVER not found - not building COVER wrapper")
    return()
endif()

#include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
#set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

#SET(EXTRA_LIBS ${EXTRA_LIBS} MPI::MPI_C)
add_definitions(-DHAS_MPI)
add_definitions(-DCOVER_PLUGIN)

vistle_add_library(
    vistle_cover
    SHARED
    COVER.cpp
    COVER.h
    VistleGeometryGenerator.h
    VistleGeometryGenerator.cpp
    HeightMap.h
    HeightMap.cpp)
vistle_export_library(
    vistle_cover
    SHARED
    COVER.cpp
    COVER.h
    VistleGeometryGenerator.h
    VistleGeometryGenerator.cpp
    HeightMap.h
    HeightMap.cpp)
target_include_directories(vistle_cover SYSTEM PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
target_include_directories(
    vistle_cover
    PRIVATE ${COVER_INCLUDE_DIRS}
    PRIVATE ${PROJECT_SOURCE_DIR}
    PRIVATE .
    PRIVATE ..)
target_link_libraries(
    vistle_cover
    PRIVATE ${EXTRA_LIBS}
    PUBLIC vistle_renderer
    PRIVATE vistle_util
    PRIVATE ${CMAKE_DL_LIBS}
    PRIVATE VistlePluginUtil
    PUBLIC ${COVER_LIBRARY}
    PUBLIC ${COVER_PLUGINUTIL_LIBRARY}
    PRIVATE ${COVISE_NET_LIBRARY}
    PRIVATE ${OPENSCENEGRAPH_LIBRARIES})

if(NOT VISTLE_MULTI_PROCESS)
    target_link_libraries(vistle_cover PRIVATE vistle_clustermanager)
endif()

add_module(COVER "VR renderer for immersive environments" coverwrapper.cpp)

target_include_directories(COVER SYSTEM PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
target_include_directories(
    COVER
    PRIVATE ${COVER_INCLUDE_DIRS}
    PRIVATE plugin
    PRIVATE .
    PRIVATE ..)
target_link_libraries(COVER vistle_cover)

if(NOT VISTLE_MULTI_PROCESS)
    if(APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${VISTLE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
    elseif(WIN32)

    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${VISTLE_SHARED_LINKER_FLAGS} -Wl,--allow-shlib-undefined")
    endif()
endif()

add_subdirectory(VistlePluginUtil)
add_subdirectory(plugin)
