vistle_add_executable(vistle hubmain.cpp)
target_link_libraries(
    vistle
    Boost::system
    Boost::program_options
    ${EXTRA_LIBRARIES}
    vistle_control
    vistle_util
    vistle_core
    vistle_net
    Threads::Threads)
