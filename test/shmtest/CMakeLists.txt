add_executable(vistle_shmtest shmtest.cpp)
target_include_directories(vistle_shmtest PRIVATE ../..)
target_link_libraries(
    vistle_shmtest
    PRIVATE vistle_core
    PRIVATE Boost::boost
    PRIVATE MPI::MPI_CXX
    PRIVATE vistle_core
    PRIVATE Threads::Threads)
