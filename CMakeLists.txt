cmake_minimum_required(VERSION 2.8)
project(vistle_all)

option(VISTLE_BUILD_SHARED "Build all libraries as static libraries instead of shared" ON)

if (VISTLE_BUILD_SHARED)
   set(VISTLE_LIB_TYPE SHARED)
else()
   set(VISTLE_LIB_TYPE STATIC)
   add_definitions(-DVISTLE_STATIC)
endif()

set(BUILD_SHARED_LIBS ${VISTLE_BUILD_SHARED})

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

   if (NOT ${BUILD_SHARED_LIBS})
      # for linking the libraries into plugins
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
   endif()

   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")

   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
      set(CUDA_HOST_COMPILER ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)

      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfxsr")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfxsr")
   endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
   set(CMAKE_CXX_FLAGS "-std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

option(VISTLE_GUI "Build Vistle GUI (requires Qt)" ON)
if(VISTLE_GUI)
set(COVISE_USE_QT4 OFF)
set(DESKVOX_USE_QT4 ${COVISE_USE_QT4})
include(cmake/Qt4-5.cmake)
find_qt()
endif()

if(WIN32)
    set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
    STRING(REGEX REPLACE "\\\\" "/" BOOST_ROOT ${BOOST_ROOT}) 
    set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/windows;${CMAKE_MODULE_PATH}")
    add_definitions("-DOMPI_IMPORTS")
    add_definitions("-DWIN32_LEAN_AND_MEAN")
    add_definitions("-DNOMINMAX")
    add_definitions("-DBOOST_ALL_NO_LIB")
	
	
endif(WIN32)

find_package(Boost 1.40 REQUIRED thread system)
add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

add_subdirectory(3rdparty)
add_subdirectory(vistle)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.2")
   include(FeatureSummary)
   feature_summary(WHAT ALL)
endif()                                                                                                                                                            
