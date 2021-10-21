add_executable(vistle_mpitest vistle_mpitest.cpp)
target_include_directories(vistle_mpitest PRIVATE ../..)
target_link_libraries(
    vistle_mpitest
    PRIVATE MPI::MPI_C
    PRIVATE vistle_util)
