vistle_add_library(vistle_libsim_connect connect.cpp connect.h export.h)
vistle_export_library(vistle_libsim_connect connect.cpp connect.h export.h)
target_link_libraries(
    vistle_libsim_connect
    PRIVATE Boost::boost
    PRIVATE Boost::system
    PRIVATE Threads::Threads)
