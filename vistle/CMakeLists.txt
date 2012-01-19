cmake_minimum_required(VERSION 2.6)
project(vistle)

find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)
find_package(OpenSceneGraph 2.8.0 COMPONENTS osgDB osgUtil osgViewer osgGA osgFX)

#add_library(object SHARED object.cpp message.cpp messagequeue.cpp)

add_executable(communicator communicator_collective.cpp message.cpp messagequeue.cpp object.cpp portmanager.cpp)
add_executable(gendat gendat.cpp module.cpp message.cpp messagequeue.cpp object.cpp)
add_executable(add add.cpp module.cpp message.cpp messagequeue.cpp object.cpp)
add_executable(osgrenderer osgrenderer.cpp renderer.cpp module.cpp message.cpp messagequeue.cpp object.cpp)

include_directories(${Boost_INCLUDE_DIRS} ${OPENSCENEGRAPH_INCLUDE_DIRS} ${MPI_INCLUDE_PATH})

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS} "-Wall")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

target_link_libraries(communicator ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(gendat ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(add ${MPI_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(osgrenderer ${MPI_LIBRARIES} ${Boost_LIBRARIES} ${OPENSCENEGRAPH_LIBRARIES})

set(CMAKE_BUILD_TYPE Debug)