set(pluginutil_SOURCES
   VistleInteractor.cpp
   VistleRenderObject.cpp
)

set(pluginutil_HEADERS
   export.h
   VistleInteractor.h
   VistleRenderObject.h
)
include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIRS}
)
include_directories(
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
