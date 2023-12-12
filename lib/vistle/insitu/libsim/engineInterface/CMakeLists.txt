if(NOT VISTLE_MULTI_PROCESS)
    vistle_add_library(vistle_libsim_engine_interface EXPORT EngineInterface.h EngineInterface.cpp)
    vistle_target_link_libraries(vistle_libsim_engine_interface PRIVATE vistle_insitu_message Threads::Threads MPI::MPI_C)
endif()
