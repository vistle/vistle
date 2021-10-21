add_executable(vistle_msgsize messagesize.cpp)
target_link_libraries(
    vistle_msgsize
    PRIVATE Boost::boost
    PRIVATE Boost::system
    PRIVATE MPI::MPI_C
    PRIVATE vistle_core)
include_directories(vistle_msgsize PRIVATE ../..)
