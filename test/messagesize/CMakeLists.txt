add_executable(vistle_msgsize messagesize.cpp)
target_link_libraries(
    vistle_msgsize
    PRIVATE Boost::boost
    PRIVATE Boost::system
    PRIVATE MPI::MPI_CXX
    PRIVATE vistle_util
    PRIVATE vistle_core)
