set(pluginutil_SOURCES VistleInteractor.cpp VistleRenderObject.cpp VistleInfo.cpp)

set(pluginutil_HEADERS export.h VistleInteractor.h VistleMessage.h VistleRenderObject.h VistleInfo.h)

set(visibility EXPORT)
if(NOT VISTLE_BUILD_SHARED)
    set(visibility INTERNAL)
endif()
vistle_add_library(VistlePluginUtil ${visibility} ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS})

target_include_directories(VistlePluginUtil SYSTEM PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS} ${COVISE_INCLUDE_DIRS} ${COVER_INCLUDE_DIRS})
target_include_directories(VistlePluginUtil PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include>)

target_link_libraries(
    VistlePluginUtil
    PRIVATE Boost::boost
    PRIVATE ${OPENSCENEGRAPH_LIBRARIES}
    PRIVATE ${COVER_LIBRARY}
    PRIVATE GLEW::GLEW
    PUBLIC vistle_core
    PUBLIC vistle_module
    PRIVATE vistle_renderer)
