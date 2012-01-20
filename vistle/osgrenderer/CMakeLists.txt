find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)
find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer)

add_executable(osgrenderer osgrenderer.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ../core
)

target_link_libraries(osgrenderer
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        vistle_core
)
