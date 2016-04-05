vistle_find_package(LibVncServer)
vistle_find_package(TBB)
vistle_find_package(JPEGTURBO)
vistle_find_package(GLEW)
using(SNAPPY optional)
using(GLEW)
using(BOOST)
if(NOT JPEGTURBO_FOUND)
   RETURN()
endif(NOT JPEGTURBO_FOUND)
if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
   return()
endif()

if(NOT LIBVNCSERVER_FOUND)
   RETURN()
endif()

if(NOT TBB_FOUND)
   return()
endif()

if(JPEGTURBO_FOUND)
   add_definitions(-DHAVE_TURBOJPEG)
endif()


include_directories(
   ${PROJECT_SOURCE_DIR}
   ${LIBVNCSERVER_INCLUDE_DIR}
   ${TBB_INCLUDE_DIR}
   ${JPEGTURBO_INCLUDE_DIR}
)

set(HEADERS
  VncClient.h
  RemoteRenderObject.h
  coRemoteCoviseInteractor.h
  MultiChannelDrawer.h
)

set(SOURCES
  VncClient.cpp
  RemoteRenderObject.cpp
  coRemoteCoviseInteractor.cpp
  MultiChannelDrawer.cpp
)

#use_openmp()

if(NOT WIN32)
   set_source_files_properties(VncClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif()

cover_add_plugin(VncClient ${HEADER} ${SOURCES})
target_link_libraries(VncClient vistle_rhr ${TURBOJPEG_LIBRARIES} ${LIBVNCSERVER_CLIENT_LIBRARY} ${TBB_LIBRARIES})
