covise_find_package(LibVncServer)
if(NOT LIBVNCSERVER_FOUND)
   return()
endif()

using(SNAPPY optional)

include_directories(${LIBVNCSERVER_INCLUDE_DIR})

set(HEADERS
  VncServer.h
)

set(SOURCES
  VncServer.cpp
)

if(CUDA_FOUND AND VISTLE_USE_CUDA)
   add_definitions(-DHAVE_CUDA)
   include_directories(${CUDA_INCLUDE_DIRS})
endif()

#use_openmp()

cover_add_plugin(VncServer ${CUDA_OBJ} ${HEADER} ${SOURCES})
target_link_libraries(VncServer vistle_rhr)
