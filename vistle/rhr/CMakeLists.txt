vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system)
vistle_find_package(CUDA)
vistle_find_package(GLEW)
vistle_find_package(OpenGL)
vistle_find_package(LibVncServer)
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

use_openmp()

if(JPEGTURBO_FOUND)
   add_definitions(-DHAVE_TURBOJPEG)
endif()

if(SNAPPY_FOUND)
   add_definitions(-DHAVE_SNAPPY)
   include_directories(${SNAPPY_INCLUDE_DIR})
endif()

set(RHR_SOURCES
   depthquant.cpp
   rhrserver.cpp
   )

set(RHR_HEADERS
   depthquant.h
   rfbext.h
   ReadBackCuda.h
   rhrserver.h
   )

if(LIBVNCSERVER_FOUND AND TBB_FOUND)
   set(RHR_SOURCES ${RHR_SOURCES} vncserver.cpp)
   set(RHR_HEADERS ${RHR_HEADERS} vncserver.h)
   include_directories(
        ${LIBVNCSERVER_INCLUDE_DIR}
   )
endif()

include_directories(
        ..
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
        ${JPEGTURBO_INCLUDE_DIR}
        ..
)

if(CUDA_FOUND AND GLEW_FOUND AND OPENGL_FOUND AND VISTLE_USE_CUDA)
  set(USE_CUDA TRUE)
  set(RHR_CUDA_SOURCES
      ReadBackCuda.cu
  )

  add_definitions(-DHAVE_CUDA)
  include_directories(${CUDA_INCLUDE_DIRS} ${GLEW_INCLUDES} ${OPENGL_INCLUDES})
  cuda_compile(RHR_CUDA_OBJ ${RHR_CUDA_SOURCES} ${VISTLE_LIB_TYPE})
endif()

if (NOT "${TURBOJPEG_LIBRARY_DYNAMIC}" STREQUAL "")
  vistle_add_library(vistle_rhr ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})
else()
  vistle_add_library(vistle_rhr STATIC ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})
endif()

if(USE_CUDA)
   target_link_libraries(vistle_rhr ${CUDA_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

target_link_libraries(vistle_rhr
        vistle_util
        ${Boost_LIBRARIES}
        ${TBB_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${TURBOJPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
)

if(SNAPPY_FOUND)
   target_link_libraries(vistle_rhr ${SNAPPY_LIBRARIES})
endif()

if(LIBVNCSERVER_FOUND)
   target_link_libraries(vistle_rhr ${LIBVNCSERVER_SERVER_LIBRARY})
endif()

add_executable(depthtest depthtest.cpp)
target_link_libraries(depthtest vistle_rhr)
