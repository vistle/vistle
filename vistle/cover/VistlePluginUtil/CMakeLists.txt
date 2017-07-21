vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system filesystem)

set(pluginutil_SOURCES
   VistleRenderObject.cpp
   MultiChannelDrawer.cpp
)

set(pluginutil_HEADERS
   export.h
   VistleRenderObject.h
   MultiChannelDrawer.h
)

include_directories(
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}
)

vistle_add_library(VistlePluginUtil ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS})

target_link_libraries(VistlePluginUtil
        ${Boost_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${COVER_LIBRARY}
        ${OPENGL_LIBRARIES}
        vistle_core
)
