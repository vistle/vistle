cmake_minimum_required(VERSION 2.6)
project(vistle)

#add_library(object SHARED object.cpp message.cpp messagequeue.cpp)

add_executable(communicator communicator_collective.cpp message.cpp messagequeue.cpp object.cpp portmanager.cpp)
add_executable(gendat gendat.cpp module.cpp message.cpp messagequeue.cpp object.cpp)
add_executable(add add.cpp module.cpp message.cpp messagequeue.cpp object.cpp)

#add_executable(testshm_master testshm_master.cpp)
#add_executable(testshm_slave testshm_slave.cpp)
#add_executable(testshm testshm.cpp)

find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} "-Wall")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
include_directories(${MPI_INCLUDE_PATH})

target_link_libraries(communicator ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(gendat ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(add ${MPI_LIBRARIES} ${Boost_LIBRARIES})

#target_link_libraries(testshm ${MPI_LIBRARIES} ${Boost_LIBRARIES} object)
#target_link_libraries(testshm_master ${MPI_LIBRARIES} ${Boost_LIBRARIES} object)
#target_link_libraries(testshm_slave ${MPI_LIBRARIES} ${Boost_LIBRARIES} object)

set(CMAKE_BUILD_TYPE Debug)