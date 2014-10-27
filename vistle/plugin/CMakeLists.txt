cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(COVER)

if(NOT COVER_FOUND)
   message("COVER not found")
   return()
endif()

message("Building Vistle plugin for ARCHSUFFIX ${COVISE_ARCHSUFFIX}")

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
        ../../3rdparty/boost-mpi/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}
)

cover_add_plugin(VistlePlugin ${HEADERS} ${SOURCES})
target_link_libraries(VistlePlugin vistle_module vistle_boost_mpi)
