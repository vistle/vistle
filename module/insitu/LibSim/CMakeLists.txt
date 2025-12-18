add_module(LibSimController "acquire data from LibSim instrumented simulations" LibSimModule.h LibSimModule.cpp)
target_link_libraries(
    LibSimController
    PRIVATE vistle_insitu_core
            vistle_insitu_message
            vistle_insitu_module
            vistle_libsim_connect
            vistle_module
            vistle_util
            Boost::boost)

target_include_directories(LibSimController PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/vistle/insitu/libsim>
                                                    $<INSTALL_INTERFACE:include/vistle/insitu/libsim>)

if(NOT VISTLE_MULTI_PROCESS)
    target_link_libraries(LibSimController PRIVATE vistle_libsim_engine_interface)
endif()
