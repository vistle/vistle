vistle_find_package(COVER)
if (NOT COVER_FOUND)
   message("COVER not found - not building COVER plugins")
   return()
endif()

message("Building COVER plugins for ARCHSUFFIX ${COVISE_ARCHSUFFIX}, destination is ${COVISE_DESTDIR}")

include_directories(${PROJECT_SOURCE_DIR}/vistle)

add_definitions(-DHAS_MPI)

add_subdirectory(VistlePluginUtil)

add_subdirectory(Vistle)
add_subdirectory(CompositorIceT)
add_subdirectory(RhrClient)
