project(vistle_covise)

set(VISTLE_BUILD ON)
set(COVISE_USE_QT4 OFF)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../vistle/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../vistle/bin)

add_definitions(-DHLRS)

INCLUDE(cmake/CoviseHelperMacros.cmake)
create_using()

find_package(MPI)
find_package(ZLIB REQUIRED)
covise_find_package(OpenSSL REQUIRED)

find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer osgGA osgDB osgFX osgUtil osgText)
find_package(OpenGL)
covise_find_package(GLEW)

if(APPLE)
  find_library(APP_SERVICES_LIBRARY ApplicationServices)
  mark_as_advanced(APP_SERVICES_LIBRARY)
endif(APPLE)

include (TestBigEndian)
test_big_endian(BIGENDIAN)
if(NOT BIGENDIAN)
   add_definitions(-DBYTESWAP)
endif()
covise_find_package(XercesC REQUIRED)

set(COVISE_USE_CUDA TRUE)
covise_find_package(CUDA)


include_directories(.)
include_directories(../vistle)

add_subdirectory(file)
add_subdirectory(grmsg)
add_subdirectory(util)
add_subdirectory(config)
add_subdirectory(net)
add_subdirectory(vrbclient)
add_subdirectory(gpu)

add_subdirectory(OpenCOVER)
