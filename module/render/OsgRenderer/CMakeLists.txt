vistle_find_package(ZLIB)
vistle_find_package(GLEW)
vistle_find_package(EGL)
if(NOT EGL_FOUND)
    vistle_find_package(X11)
endif()

if(NOT OPENSCENEGRAPH_FOUND OR NOT OPENGL_FOUND)
    return()
endif()

if(NOT TARGET GLEW::GLEW)
    message("OsgRenderer: GLEW not found")
    return()
endif()

set(EGL_SOURCES)
if(EGL_FOUND)
    add_definitions(-DHAVE_EGL)
    set(EGL_SOURCES EGLWindow.h EGLWindow.cpp)
endif()

add_module(
    OsgRenderer
    "OpenSceneGraph remote renderer"
    OSGRenderer.cpp
    OSGRenderer.h
    ../COVER/VistleGeometryGenerator.cpp
    EnableGLDebugOperation.cpp
    EnableGLDebugOperation.h
    ${EGL_SOURCES})

if(WIN32)

else(WIN32)
    set_source_files_properties(OSGRenderer.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(SYSTEM ${OPENSCENEGRAPH_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS})
include_directories(../COVER)
if(EGL_FOUND)
    include_directories(SYSTEM ${EGL_INCLUDE_DIRS})
    target_link_libraries(OsgRenderer ${EGL_LIBRARIES})
elseif(NOT APPLE AND X11_FOUND)
    add_definitions(-DUSE_X11)
    include_directories(SYSTEM ${X11_INCLUDES})
    target_link_libraries(OsgRenderer ${X11_LIBRARIES})
endif()

target_link_libraries(
    OsgRenderer
    ${OPENSCENEGRAPH_LIBRARIES}
    GLEW::GLEW
    ${OPENGL_LIBRARIES}
    vistle_module
    vistle_renderer
    vistle_rhr
    ${ZLIB_LIBRARIES})
