set(pluginutil_SOURCES VistleInteractor.cpp VistleRenderObject.cpp)

set(pluginutil_HEADERS export.h VistleInteractor.h VistleMessage.h VistleRenderObject.h)

add_definitions(-DHAS_MPI)

vistle_add_library(VistlePluginUtil ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS})
vistle_export_library(VistlePluginUtil ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS})

target_include_directories(
    VistlePluginUtil SYSTEM
    PRIVATE ${MPI_C_INCLUDE_PATH}
    PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS}
    PRIVATE ${GLEW_INCLUDE_DIR}
    PRIVATE ${OPENGL_INCLUDE_DIRS})
target_include_directories(
    VistlePluginUtil
    PRIVATE ${COVISE_INCLUDE_DIRS}
    PRIVATE ${COVER_INCLUDE_DIRS})

target_link_libraries(
    VistlePluginUtil
    PRIVATE Boost::boost
    PRIVATE ${OPENSCENEGRAPH_LIBRARIES}
    PRIVATE ${COVER_LIBRARY}
    PRIVATE ${OPENGL_LIBRARIES}
    PRIVATE vistle_core
    PRIVATE vistle_module
    PRIVATE vistle_renderer)
