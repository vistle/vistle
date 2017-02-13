#vistle_find_package(LibVncServer)
vistle_find_package(TBB)

set(renderer_SOURCES
   renderer.cpp
   renderobject.cpp
)

set(renderer_HEADERS
   export.h
   renderer.h
   renderobject.h
)

if(TBB_FOUND)
   set(renderer_SOURCES ${renderer_SOURCES}
      rhrcontroller.cpp
      parrendmgr.cpp
   )
   set(renderer_HEADERS ${renderer_HEADERS}
      parrendmgr.h
      rhrcontroller.h
   )
   include_directories(
      ${TBB_INCLUDE_DIR}
      ${ICET_INCLUDE_DIRS}
   )
endif()

if(LIBVNCSERVER_FOUND)
   set(renderer_SOURCES ${renderer_SOURCES} vnccontroller.cpp)
   set(renderer_HEADERS ${renderer_HEADERS} vnccontroller.h)
   include_directories(${LIBVNCSERVER_INCLUDE_DIR})
endif()

use_openmp()

if (JPEGTURBO_FOUND AND ("${TURBOJPEG_LIBRARY_DYNAMIC}" STREQUAL ""))
  vistle_add_library(vistle_renderer STATIC ${renderer_SOURCES} ${renderer_HEADERS})
else()
  vistle_add_library(vistle_renderer ${VISTLE_LIB_TYPE} ${renderer_SOURCES} ${renderer_HEADERS})
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_renderer
		-lrt
	)
endif()

target_link_libraries(vistle_renderer
        vistle_util
        vistle_core
        vistle_module
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
)
if(TBB_FOUND)
   target_link_libraries(vistle_renderer
      vistle_rhr
      ${ICET_CORE_LIBS}
      ${ICET_MPI_LIBS}
   )
endif()

if(LIBVNCSERVER_FOUND)
   target_link_libraries(vistle_renderer ${LIBVNCSERVER_SERVER_LIBRARY})
endif()

include_directories(
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

add_subdirectory(COVER)
add_subdirectory(OsgRenderer)
add_subdirectory(DisCOVERay)
