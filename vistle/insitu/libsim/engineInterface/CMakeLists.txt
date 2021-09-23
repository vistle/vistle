if(NOT VISTLE_MULTI_PROCESS)
    vistle_add_library(vistle_libsim_engine_interface EngineInterface.h EngineInterface.cpp)
    vistle_export_library(vistle_libsim_engine_interface EngineInterface.h EngineInterface.cpp)
    vistle_target_link_libraries(
        vistle_libsim_engine_interface
        PRIVATE
        Boost::boost
        PRIVATE
        Boost::system
        PRIVATE
        Threads::Threads)
endif()
