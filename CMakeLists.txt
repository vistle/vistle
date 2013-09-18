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

   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -mfxsr")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -mfxsr")

   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
      set(CUDA_HOST_COMPILER ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)
   endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

set(COVISE_USE_QT4 OFF)
set(DESKVOX_USE_QT4 ${COVISE_USE_QT4})
include(cmake/Qt4-5.cmake)
find_qt()

add_subdirectory(3rdparty)
add_subdirectory(vistle)
