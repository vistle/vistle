add_executable(mpibcast mpibcast.cpp)
target_link_libraries(
    mpibcast
    PRIVATE Boost::boost
    PRIVATE MPI::MPI_C
    PRIVATE vistle_boost_mpi)

target_include_directories(mpibcast PRIVATE ../..)
