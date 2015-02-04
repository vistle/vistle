vistle_find_package(LibVncServer)
vistle_find_package(embree)                                                                                                     
vistle_find_package(TBB)
vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)
vistle_find_package(SNAPPY)

if(NOT JPEGTURBO_FOUND)
   RETURN()
endif(NOT JPEGTURBO_FOUND)
if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
   message("Ray: turbojpeg w/o tjMcuWidth")
   return()
endif()

if(NOT LIBVNCSERVER_FOUND)
   message("Ray: libvncserver not found")
   return()
endif()

if(NOT SNAPPY_FOUND)
   message("Ray: Snappy not found")
   return()
endif()

if(NOT EMBREE_FOUND)
   message("Ray: Embree not found")
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

add_module(Ray
   renderobject.cpp
   ray.cpp
   )

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${ICET_INCLUDE_DIRS}
        ${LIBVNCSERVER_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}
        ${EMBREE_INCLUDE_PATH}
        ${TBB_INCLUDE_DIRS}
        ${JPEGTURBO_INCLUDE_DIR}
)

target_link_libraries(Ray
        ${Boost_LIBRARIES}
        vistle_boost_mpi
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_rhr
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
        ${LIBVNCSERVER_SERVER_LIBRARY}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${SNAPPY_LIBRARIES}
        ${EMBREE_LIBRARIES}
        ${TBB_LIBRARIES}
)
