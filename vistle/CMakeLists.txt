cmake_minimum_required(VERSION 2.8)
project(vistle)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH}")

enable_testing()

option(TEMPLATES_IN_HEADERS "define templates in headers" ON)
if(${TEMPLATES_IN_HEADERS})
   add_definitions(-DTEMPLATES_IN_HEADERS)
   add_definitions(-DVISTLE_IMPL)
endif()

macro(USE_OPENMP)
    if(NOT APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgomp")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
    endif()
endmacro(USE_OPENMP)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=512")
   #if(APPLE)
   #   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   #endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
   #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
endif()

if(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall")
else(WIN32)
   set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall -Wno-unknown-pragmas")
endif(WIN32)

#set(CMAKE_BUILD_TYPE Debug)

find_package(TBB)
find_package(Thrust)

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_PATH})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(util)
add_subdirectory(core)
add_subdirectory(control)
add_subdirectory(userinterface)

add_subdirectory(vistle)
add_subdirectory(blower)

add_subdirectory(module)

add_subdirectory(plugin)

add_subdirectory(gui)

add_subdirectory(test)
