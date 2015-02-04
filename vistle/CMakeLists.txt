project(vistle)

if(POLICY CMP0043)
   # configuration (RelWithDebInfo, Debug, ...) dependent COMPILE_DEFINITIONS are not used
   # - default to new behavior
   cmake_policy(SET CMP0043 NEW)
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH}")

enable_testing()

option(TEMPLATES_IN_HEADERS "define templates in headers" ON)
if(${TEMPLATES_IN_HEADERS})
   add_definitions(-DTEMPLATES_IN_HEADERS)
   add_definitions(-DVISTLE_IMPL)
endif()

find_package(OpenMP)
macro(USE_OPENMP)
   if (OPENMP_FOUND)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
   else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
   endif()
endmacro(USE_OPENMP)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=512")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

if(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall")
else(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall -Wno-unknown-pragmas")
endif(WIN32)

#set(CMAKE_BUILD_TYPE Debug)

vistle_find_package(PythonLibs)

vistle_find_package(TBB)
#vistle_find_package(Thrust REQUIRED)
set(THRUST_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../3rdparty/thrust")

#vistle_find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_PATH "../3rdparty")
include_directories(${EIGEN3_INCLUDE_PATH})

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{EXTERNLIBS}/openscenegraph")
vistle_find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer osgGA osgDB osgUtil
   osgText osgParticle osgSim osgFX)
vistle_find_package(OpenGL)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(rhr)
add_subdirectory(control)
add_subdirectory(userinterface)

add_subdirectory(bin)
add_subdirectory(manager)
add_subdirectory(hub)
add_subdirectory(blower)

add_subdirectory(module)

add_subdirectory(cover)

add_subdirectory(gui)

add_subdirectory(test)
