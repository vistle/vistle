find_package(Boost 1.40 REQUIRED COMPONENTS chrono system)
find_package(CUDA)
find_package(GLEW)
find_package(OpenGL)
find_package(LibVncServer)
find_package(TBB)
find_package(ZLIB)
find_package(JPEGTURBO)
find_package(SNAPPY)

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


set(RHR_SOURCES
   depthquant.cpp
   vncserver.cpp
   )

set(RHR_HEADERS
   depthquant.h
   rfbext.h
   vncserver.h
   ReadBackCuda.h
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

add_library(vistle_rhr ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})

if(USE_CUDA)
   target_link_libraries(vistle_rhr ${CUDA_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

target_link_libraries(vistle_rhr
        vistle_util
        ${Boost_LIBRARIES}
        ${LIBVNCSERVER_SERVER_LIBRARY}
        ${SNAPPY_LIBRARIES}
        ${TBB_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${LIBVNCSERVER_INCLUDE_DIR}
        ${SNAPPY_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${JPEGTURBO_INCLUDE_DIR}
        ..
)

add_executable(depthtest depthtest.cpp)
target_link_libraries(depthtest vistle_rhr)
