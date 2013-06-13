cmake_minimum_required(VERSION 2.6)
project(vistle_all)

enable_testing()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

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

add_subdirectory(covise)
add_subdirectory(3rdparty)
add_subdirectory(vistle)
