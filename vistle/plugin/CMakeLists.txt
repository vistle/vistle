set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/OpenCOVER/plugins)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{EXTERNLIBS}/OpenSceneGraph")

find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer osgGA osgDB)
find_package(OpenGL)

if(NOT OPENSCENEGRAPH_FOUND OR NOT OPENGL_FOUND)
    return()
endif()

add_library(Vistle SHARED
   VistlePlugin.cpp
   VistleRenderObject.cpp
   VistleGeometryGenerator.cpp
   )

if(WIN32)
else(WIN32)
set_source_files_properties(VistlePlugin.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../covise/OpenCOVER
        ${PROJECT_SOURCE_DIR}/../covise
)

target_link_libraries(Vistle
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${OPENSCENEGRAPH_LIBRARIES}
        ${OPENGL_LIBRARIES}
        vistle_core
        covise_cover
)

