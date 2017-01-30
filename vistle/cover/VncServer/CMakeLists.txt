return()

vistle_find_package(LibVncServer)
vistle_find_package(TBB)
if(NOT LIBVNCSERVER_FOUND)
   return()
endif()
if(NOT TBB_FOUND)
   return()
endif()


using(SNAPPY optional)

include_directories(
   ${PROJECT_SOURCE_DIR}
   ${LIBVNCSERVER_INCLUDE_DIR}
   ${TBB_INCLUDE_DIR}
)

set(HEADERS
  VncServerPlugin.h
)

set(SOURCES
  VncServerPlugin.cpp
)

if(CUDA_FOUND AND VISTLE_USE_CUDA)
   add_definitions(-DHAVE_CUDA)
   include_directories(${CUDA_INCLUDE_DIRS})
endif()

#use_openmp()

if(NOT WIN32)
   set_source_files_properties(VncServerPlugin.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif()

cover_add_plugin(VncServer ${CUDA_OBJ} ${HEADER} ${SOURCES})
target_link_libraries(VncServer vistle_rhr)
