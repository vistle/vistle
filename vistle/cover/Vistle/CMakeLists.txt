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
        ../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}
)

add_definitions(-DCOVER_PLUGIN)

cover_add_plugin(VistlePlugin ${HEADERS} ${SOURCES})
target_link_libraries(VistlePlugin vistle_renderer vistle_boost_mpi)
