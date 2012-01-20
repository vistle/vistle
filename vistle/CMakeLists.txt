cmake_minimum_required(VERSION 2.6)
project(vistle)

#find_package(MPI REQUIRED)
#find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} "-Wall")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
set(CMAKE_BUILD_TYPE Debug)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(core)

add_subdirectory(communicator)

add_subdirectory(gendat)
add_subdirectory(osgrenderer)