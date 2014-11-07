cmake_minimum_required(VERSION 2.8)
project(vistle_all)

# vim's YouCompleteMe can make use of this
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(VISTLE_BUILD_SHARED "Build all libraries as shared libraries instead of static" ON)

option(VISTLE_SANITIZE_UNDEFINED "Build with -fsanitize=undefined" OFF)
option(VISTLE_SANITIZE_ADDRESS "Build with -fsanitize=address" OFF)
option(VISTLE_SANITIZE_THREAD "Build with -fsanitize=thread" OFF)

if (NOT "$ENV{VBUILD}" STREQUAL "")
   include("${PROJECT_SOURCE_DIR}/build/$ENV{VBUILD}.cmake")
endif()

if (VISTLE_BUILD_SHARED)
   set(VISTLE_LIB_TYPE SHARED)
else()
   set(VISTLE_LIB_TYPE STATIC)
   add_definitions(-DVISTLE_STATIC)
endif()
set(BUILD_SHARED_LIBS ${VISTLE_BUILD_SHARED})

# disable rpath - only rely on LD_LIBRARY_PATH and the likes
set(CMAKE_SKIP_RPATH TRUE)

# allow for optimizations in Boost.MPI
add_definitions(-DBOOST_MPI_HOMOGENEOUS)

enable_testing()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
   add_definitions(-DNDEBUG)
endif()

#add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=short)
add_definitions(-DEIGEN_DEFAULT_DENSE_INDEX_TYPE=int)

set(ASAN_COMPILE_FLAGS "")
set(ASAN_LINK_FLAGS "")
if (VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=undefined")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=undefined")
endif()
if (VISTLE_SANITIZE_ADDRESS)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=address")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=address")
endif()
if (VISTLE_SANITIZE_THREAD)
   set(ASAN_LINK_FLAGS "${ASAN_LINK_FLAGS} -fsanitize=thread")
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fsanitize=thread")
endif()
if (VISTLE_SANITIZE_THREAD OR VISTLE_SANITIZE_ADDRESS OR VISTLE_SANITIZE_UNDEFINED)
   set(ASAN_COMPILE_FLAGS "${ASAN_COMPILE_FLAGS} -fno-omit-frame-pointer")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${ASAN_LINK_FLAGS}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   if(APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
   #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
   if (NOT APPLE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
   endif()

   if (NOT ${BUILD_SHARED_LIBS})
      # for linking the libraries into plugins
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
   endif()

   if (NOT APPLE)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
   endif()

   execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
      set(CUDA_HOST_COMPILER ${CMAKE_SOURCE_DIR}/scripts/cuda-host-compiler CACHE STRING "CUDA nvcc host compiler" FORCE)

      if (NOT APPLE)
         set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfxsr")
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfxsr")
      endif()
   endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
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
if (Boost_VERSION VERSION_GREATER 105599)
   set(BOOST_MPI_DIR "boost-mpi")
else()
   set(BOOST_MPI_DIR "boost-mpi-1.55")
endif()
#message("Using Boost.MPI from ${BOOST_MPI_DIR} for Boost ${Boost_VERSION}")

find_package(MPI REQUIRED)
include_directories(${MPI_C_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS}")
add_definitions(-DOMPI_SKIP_MPICXX)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")

add_subdirectory(3rdparty)
add_subdirectory(vistle)

if(${CMAKE_VERSION} VERSION_GREATER "2.8.2")
   include(FeatureSummary)
   feature_summary(WHAT ALL)
endif()                                                                                                                                                            
