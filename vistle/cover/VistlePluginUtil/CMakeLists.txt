vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system filesystem)
using(GLEW)

vistle_find_package(glm)

set(pluginutil_SOURCES
   VistleRenderObject.cpp
   MultiChannelDrawer.cpp
   CubemapReprojection/CubemapReprojector.cpp
)

set(pluginutil_HEADERS
   export.h
   VistleRenderObject.h
   MultiChannelDrawer.h
   CubemapReprojection/CubemapReprojector.h
)

include_directories(
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}
        CubemapReprojection
)

vistle_add_library(VistlePluginUtil ${VISTLE_LIB_TYPE} ${pluginutil_SOURCES} ${pluginutil_HEADERS})

target_link_libraries(VistlePluginUtil
        ${Boost_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${COVER_LIBRARY}
        vistle_core
)
