vistle_find_package(OpenGL)
vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)
#vistle_find_package(ZFP)

#use_openmp()

set(RHR_SOURCES compdecomp.cpp depthquant.cpp predict.cpp rfbext.cpp)

set(RHR_HEADERS compdecomp.h depthquant.h predict.h rfbext.h ReadBackCuda.h)

set(RHR_SOURCES ${RHR_SOURCES} rhrserver.cpp)
set(RHR_HEADERS ${RHR_HEADERS} rhrserver.h)

set(RHR_CUDA_SOURCES ReadBackCuda.cu)

if(CUDA_FOUND
   AND GLEW_FOUND
   AND OPENGL_FOUND
   AND VISTLE_USE_CUDA)
    set(USE_CUDA TRUE)

    add_definitions(-DHAVE_CUDA)
    include_directories(SYSTEM ${CUDA_INCLUDE_DIRS} ${GLEW_INCLUDES} ${OPENGL_INCLUDES})
    cuda_compile(RHR_CUDA_OBJ ${RHR_CUDA_SOURCES} ${VISTLE_LIB_TYPE})
else()
    add_custom_target(RHR_CUDA SOURCES ${RHR_CUDA_SOURCES})
endif()

vistle_add_library(vistle_rhr ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})
vistle_export_library(vistle_rhr ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS} ${RHR_CUDA_OBJ})

target_link_libraries(
    vistle_rhr
    PUBLIC vistle_util
    PUBLIC vistle_core
    PRIVATE vistle_module
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE ${JPEG_LIBRARIES})

if(USE_CUDA)
    target_link_libraries(
        vistle_rhr
        PRIVATE ${CUDA_LIBRARIES}
        PRIVATE ${GLEW_LIBRARIES}
        PRIVATE ${OPENGL_LIBRARIES})
endif()

if(JPEGTURBO_FOUND)
    if(NOT TURBOJPEG_HAVE_TJMCUWIDTH)
        message("vistle_rhr: turbojpeg w/o tjMcuWidth")
    else()
        target_compile_definitions(vistle_rhr PRIVATE HAVE_TURBOJPEG)
        target_include_directories(vistle_rhr SYSTEM PRIVATE ${JPEGTURBO_INCLUDE_DIR})
    endif()

    target_link_libraries(vistle_rhr PRIVATE ${TURBOJPEG_LIBRARIES})
endif()
if(ZLIB_FOUND)
    target_link_libraries(vistle_rhr PRIVATE ${ZLIB_LIBRARIES})
endif()

if(ZFP_FOUND)
    target_compile_definitions(vistle_rhr PRIVATE HAVE_ZFP)
    target_include_directories(vistle_rhr SYSTEM PRIVATE ${ZFP_INCLUDE_DIRS})
    target_link_libraries(vistle_rhr PRIVATE ${ZFP_LIBRARIES})
endif()

add_executable(depthtest depthtest.cpp depthcompare.cpp)
target_link_libraries(depthtest PRIVATE vistle_rhr)

add_executable(depthbench depthbench.cpp depthcompare.cpp)
target_link_libraries(depthbench PRIVATE vistle_rhr)
