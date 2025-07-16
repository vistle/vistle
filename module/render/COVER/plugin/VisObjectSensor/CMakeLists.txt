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

using(VRML)

set(HEADERS VisObjectSensor.h)
set(SOURCES VisObjectSensor.cpp)

cover_add_plugin(VisObjectSensor ${HEADERS} ${SOURCES})

target_include_directories(VisObjectSensor PRIVATE ..) # for VistlePluginUtil
target_link_libraries(VisObjectSensor Vrml97Cover VistlePluginUtil vistle_util vistle_core)
