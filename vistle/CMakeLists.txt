cmake_minimum_required(VERSION 2.8)
project(vistle)

enable_testing()

macro(USE_OPENMP)
    if(NOT APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgomp")
    endif()
endmacro(USE_OPENMP)

if(WIN32)
    set(Boost_USE_STATIC_LIBS true)
    set(BOOST_ROOT "$ENV{EXTERNLIBS}/boost")
    set(MPI_HOME "$ENV{EXTERNLIBS}/OpenMPI")
    set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/windows;${CMAKE_MODULE_PATH}")
    add_definitions("-DOMPI_IMPORTS")
endif(WIN32)

find_package(Boost 1.40 REQUIRED COMPONENTS date_time)
add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_DIR})
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

if(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} "-Wall")
else(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} "-Wall -Wno-unknown-pragmas")
endif(WIN32)

if (CMAKE_COMPILER_IS_GNUCC)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fvisibility=hidden")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(control)
add_subdirectory(vistle)
add_subdirectory(tools)
add_subdirectory(module)
