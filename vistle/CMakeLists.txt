cmake_minimum_required(VERSION 2.6)
project(vistle)

add_executable(communicator communicator.cpp message.cpp)
add_executable(module module.cpp)

find_package(MPI REQUIRED)

set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
include_directories(${MPI_INCLUDE_PATH})

target_link_libraries(communicator ${MPI_LIBRARIES})
target_link_libraries(module ${MPI_LIBRARIES})

set(CMAKE_BUILD_TYPE Debug)