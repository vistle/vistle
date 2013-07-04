set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "$ENV{EXTERNLIBS}/OpenSceneGraph")
find_package(OpenSceneGraph 2.8.0 COMPONENTS osgViewer osgGA osgDB)
find_package(OpenGL)

set(SOURCES
   VistlePlugin.cpp
   VistleRenderObject.cpp
   VistleGeometryGenerator.cpp
   VistleInteractor.cpp
   )

if(WIN32)
else(WIN32)
set_source_files_properties(VistlePlugin.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../covise/OpenCOVER
        ${PROJECT_SOURCE_DIR}/../covise
)

add_cover_plugin(VistlePlugin vistle_module)
