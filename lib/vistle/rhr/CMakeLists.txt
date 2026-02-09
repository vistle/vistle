vistle_find_package(ZLIB)
vistle_find_package(JPEGTURBO)

set(RHR_SOURCES compdecomp.cpp depthquant.cpp predict.cpp rfbext.cpp)

set(RHR_HEADERS compdecomp.h depthquant.h predict.h rfbext.h)

set(RHR_SOURCES ${RHR_SOURCES} rhrserver.cpp)
set(RHR_HEADERS ${RHR_HEADERS} rhrserver.h)

vistle_add_library(vistle_rhr EXPORT ${VISTLE_LIB_TYPE} ${RHR_SOURCES} ${RHR_HEADERS})

target_link_libraries(
    vistle_rhr
    PUBLIC vistle_util
    PUBLIC vistle_core
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE ${JPEG_LIBRARIES})

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

target_include_directories(vistle_rhr SYSTEM PRIVATE ${ZFP_INCLUDE_DIRS})
target_link_libraries(vistle_rhr PRIVATE ${ZFP_LIBRARIES})

add_executable(depthtest depthtest.cpp depthcompare.cpp)
target_link_libraries(depthtest PRIVATE vistle_rhr)

add_executable(depthbench depthbench.cpp depthcompare.cpp)
target_link_libraries(depthbench PRIVATE vistle_rhr)
