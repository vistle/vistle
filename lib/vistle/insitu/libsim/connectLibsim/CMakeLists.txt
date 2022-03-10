vistle_add_library(vistle_libsim_connect EXPORT connect.cpp connect.h export.h)

target_link_libraries(
    vistle_libsim_connect
    PRIVATE Boost::boost
    PRIVATE Boost::system
    PRIVATE Threads::Threads)
