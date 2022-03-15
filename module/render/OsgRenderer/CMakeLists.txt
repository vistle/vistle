vistle_find_package(ZLIB)
vistle_find_package(X11)
vistle_find_package(GLEW)

if(NOT OPENSCENEGRAPH_FOUND OR NOT OPENGL_FOUND)
    return()
endif()

if(NOT GLEW_FOUND)
    message("OsgRenderer: GLEW not found")
    return()
endif()

set(USE_TBB FALSE)
if(OPENMP_FOUND)
    use_openmp()
elseif(TBB_FOUND)
    add_definitions(-DUSE_TBB)
    include_directories(SYSTEM ${TBB_INCLUDE_DIRS})
    set(USE_TBB TRUE)
endif()

add_module(OsgRenderer "OpenSceneGraph remote renderer" OSGRenderer.cpp ../COVER/VistleGeometryGenerator.cpp EnableGLDebugOperation.cpp)

if(WIN32)

else(WIN32)
    set_source_files_properties(OSGRenderer.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(SYSTEM ${OPENSCENEGRAPH_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIRS})
include_directories(../COVER)
if(NOT APPLE AND X11_FOUND)
    add_definitions(-DUSE_X11)
    include_directories(SYSTEM ${X11_INCLUDES})
    target_link_libraries(OsgRenderer ${X11_LIBRARIES})
endif()

target_link_libraries(
    OsgRenderer
    ${OPENSCENEGRAPH_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    vistle_module
    vistle_renderer
    vistle_rhr
    ${ZLIB_LIBRARIES})

if(USE_TBB)
    target_link_libraries(OsgRenderer ${TBB_LIBRARIES})
endif()
