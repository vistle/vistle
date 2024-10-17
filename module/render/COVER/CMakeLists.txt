vistle_find_package(COVER)
if(NOT COVER_FOUND)
    message("COVER not found - not building COVER wrapper")
    return()
endif()

add_definitions(-DHAS_MPI)
add_definitions(-DCOVER_PLUGIN)

set(visibility EXPORT)
set(vistle_install_include_directory "module/render/COVER")
if(NOT VISTLE_BUILD_SHARED)
    set(visibility INTERNAL)
endif()
vistle_add_library(
    vistle_cover
    ${visibility}
    SHARED
    COVER.cpp
    COVER.h
    VistleGeometryGenerator.h
    VistleGeometryGenerator.cpp
    CoverConfigBridge.h
    CoverConfigBridge.cpp
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
    PUBLIC ${COVER_CONFIG_LIBRARY}
    PRIVATE ${COVISE_NET_LIBRARY}
    PRIVATE ${OPENSCENEGRAPH_LIBRARIES})
target_link_libraries(vistle_cover PRIVATE vistle_config)
target_link_libraries(vistle_cover PRIVATE vistle_clustermanager)

add_module(COVER "VR renderer for immersive environments" coverwrapper.cpp)

target_include_directories(COVER SYSTEM PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
target_include_directories(
    COVER
    PRIVATE ${COVER_INCLUDE_DIRS}
    PRIVATE plugin
    PRIVATE .
    PRIVATE ..)
target_link_libraries(COVER vistle_cover)

if(VISTLE_MULTI_PROCESS)
    add_module2(COVER_plugin FALSE "VR renderer for immersive environments" coverwrapper.cpp)
    target_compile_definitions(COVER_plugin PRIVATE MODULE_THREAD)
    target_include_directories(COVER_plugin SYSTEM PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})
    target_include_directories(
        COVER_plugin
        PRIVATE ${COVER_INCLUDE_DIRS}
        PRIVATE plugin
        PRIVATE .
        PRIVATE ..)
    target_link_libraries(COVER_plugin vistle_cover)
    set_target_properties(COVER_plugin PROPERTIES OUTPUT_NAME "COVER")
endif()

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
