set(SOURCES
   VistlePlugin.cpp
   )

if(WIN32)
else(WIN32)
set_source_files_properties(VistlePlugin.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)

include_directories(
        .. # VistlePluginUtil
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${COVISE_INCLUDE_DIRS}
        ${COVER_INCLUDE_DIRS}
)

add_definitions(-DCOVER_PLUGIN)

cover_add_plugin(VistlePlugin ${HEADERS} ${SOURCES})
target_link_libraries(VistlePlugin VistlePluginUtil vistle_cover vistle_renderer vistle_boost_mpi)
set_target_properties(VistlePlugin PROPERTIES OUTPUT_NAME "Vistle")
