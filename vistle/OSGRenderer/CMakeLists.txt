set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{EXTERNLIBS}/OpenSceneGraph")

find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)
find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer osgGA osgDB)

#find_package(IceT REQUIRED)
set(ICET_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/3rdparty/IceT/src/include;${CMAKE_BINARY_DIR}/3rdparty/IceT/src/include)
set(ICET_CORE_LIBS "IceTCore")
set(ICET_MPI_LIBS "IceTMPI")

if(NOT OPENSCENEGRAPH_FOUND)
    return()
endif()

add_executable(OSGRenderer OSGRenderer.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${ICET_INCLUDE_DIRS}
        ../core
)

target_link_libraries(OSGRenderer
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        vistle_core
        ${ICET_CORE_LIBS}
        ${ICET_MPI_LIBS}
)
