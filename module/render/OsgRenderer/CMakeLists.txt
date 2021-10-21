vistle_find_package(ZLIB)
vistle_find_package(X11)
vistle_find_package(GLEW)

if(NOT OPENSCENEGRAPH_FOUND OR NOT OPENGL_FOUND)
    return()
endif()

if(NOT TBB_FOUND)
    message("OsgRenderer: TBB not found")
    return()
endif()

if(NOT GLEW_FOUND)
    message("OsgRenderer: GLEW not found")
    return()
endif()

if(TBB_FOUND)
    #if (NOT OPENMP_FOUND)
    add_definitions(-DUSE_TBB)
    #endif()
else()
    use_openmp()
endif()

add_module(OsgRenderer "OpenSceneGraph remote renderer" OSGRenderer.cpp ../COVER/VistleGeometryGenerator.cpp EnableGLDebugOperation.cpp)

if(WIN32)

else(WIN32)
    set_source_files_properties(OSGRenderer.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(SYSTEM ${OPENSCENEGRAPH_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIRS} ${ICET_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})
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
    ${ICET_CORE_LIBS}
    ${ICET_MPI_LIBS}
    ${ZLIB_LIBRARIES}
    ${TBB_LIBRARIES})
