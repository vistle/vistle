vistle_find_package(LibVncServer)
vistle_find_package(TBB)
vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)
vistle_find_package(SNAPPY)
vistle_find_package(X11)

if(NOT OPENSCENEGRAPH_FOUND OR NOT OPENGL_FOUND)
    return()
endif()

if(NOT JPEGTURBO_FOUND)
   return()
endif(NOT JPEGTURBO_FOUND)
if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
   message("OsgRenderer: turbojpeg w/o tjMcuWidth")
   return()
endif()

if(NOT LIBVNCSERVER_FOUND)
   message("OsgRenderer: libvncserver not found")
   return()
endif()

if(NOT SNAPPY_FOUND)
   message("OsgRenderer: Snappy not found")
   return()
endif()

if(TBB_FOUND)
    #if (NOT OPENMP_FOUND)
      add_definitions(-DUSE_TBB)
      #endif()
else()
   use_openmp()
endif()

if(JPEGTURBO_FOUND)
   add_definitions(-DHAVE_TURBOJPEG)
endif()


add_module(OsgRenderer
   OSGRenderer.cpp
   ../../cover/Vistle/VistleGeometryGenerator.cpp
   EnableGLDebugOperation.cpp
   )

if(WIN32)
else(WIN32)
set_source_files_properties(OSGRenderer.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIRS}
        ${ICET_INCLUDE_DIRS}
        ${LIBVNCSERVER_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${JPEGTURBO_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}
        ../../cover/Vistle
)
if(NOT APPLE AND X11_FOUND)
   add_definitions(-DUSE_X11)
   include_directories(${X11_INCLUDES})
   target_link_libraries(OsgRenderer ${X11_LIBRARIES})
endif()

target_link_libraries(OsgRenderer
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${GLEW_LIBRARY}
        ${OPENGL_LIBRARIES}
        vistle_module
        vistle_renderer
        vistle_rhr
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
        ${LIBVNCSERVER_SERVER_LIBRARY}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${SNAPPY_LIBRARIES}
        ${TBB_LIBRARIES}
)
