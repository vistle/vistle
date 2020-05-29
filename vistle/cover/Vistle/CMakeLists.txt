set(SOURCES
   VistlePlugin.cpp
   )

if(WIN32)
else(WIN32)
set_source_files_properties(VistlePlugin.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif(WIN32)



cover_add_plugin(VistlePlugin ${HEADERS} ${SOURCES})
target_link_libraries(VistlePlugin 
    VistlePluginUtil 
    vistle_cover 
    vistle_renderer 
    vistle_boost_mpi)

set_target_properties(VistlePlugin PROPERTIES OUTPUT_NAME "Vistle")
target_include_directories(VistlePlugin
        PRIVATE ${COVISE_INCLUDE_DIRS}
        PRIVATE ${COVER_INCLUDE_DIRS}
        PRIVATE .. # VistlePluginUtil
)

target_compile_definitions(VistlePlugin 
    PRIVATE -DCOVER_PLUGIN)