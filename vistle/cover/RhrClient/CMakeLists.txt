vistle_find_package(GLEW)
using(GLEW)

set(HEADERS DecodeTask.h RhrClient.h RemoteConnection.h TileMessage.h NodeConfig.h)

set(SOURCES DecodeTask.cpp RhrClient.cpp RemoteConnection.cpp TileMessage.cpp NodeConfig.cpp)

#use_openmp()

if(NOT WIN32)
    set_source_files_properties(RhrClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif()

cover_add_plugin(RhrClient ${HEADER} ${SOURCES})

target_include_directories(RhrClient PRIVATE .. # for VistlePluginUtil
)
target_link_libraries(RhrClient VistlePluginUtil ${COVISE_NET_LIBRARY} vistle_rhr ${BOOST_MPI})
if(UNIX)
    target_link_libraries(RhrClient Threads::Threads)
endif()
