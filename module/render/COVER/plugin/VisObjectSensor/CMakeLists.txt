if(NOT COVER_VRML_LIBRARY OR NOT COVER_VRMLINTERFACE_LIBRARY)
    return()
endif()

# provide dependencies checked in using(VRML)
foreach(
    P
    JPEGTURBO
    JPEG
    PNG
    TIFF
    ZLIB
    FLEX
    BISON)
    covise_find_package(${P})
endforeach()

set(HEADERS VisObjectSensor.h)
set(SOURCES VisObjectSensor.cpp)

cover_add_plugin(VisObjectSensor ${HEADERS} ${SOURCES})

target_include_directories(VisObjectSensor PRIVATE ..) # for VistlePluginUtil
target_link_libraries(VisObjectSensor ${COVER_VRMLINTERFACE_LIBRARY} ${COVER_VRML_LIBRARY} VistlePluginUtil vistle_util vistle_core)
