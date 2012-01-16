cmake_minimum_required(VERSION 2.6)
project(vistle)

add_executable(communicator communicator_collective.cpp message.cpp messagequeue.cpp object.cpp portmanager.cpp)
add_executable(gendat gendat.cpp module.cpp message.cpp messagequeue.cpp object.cpp)

find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} "-Wall")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
include_directories(${MPI_INCLUDE_PATH})

target_link_libraries(communicator ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(gendat ${MPI_LIBRARIES} ${Boost_LIBRARIES})

set(CMAKE_BUILD_TYPE Debug)