add_executable(vistle_shminfo vistle_shminfo.cpp)
target_link_libraries(vistle_shminfo Boost::boost MPI::MPI_C vistle_util_mpi vistle_core)

target_include_directories(vistle_shminfo PRIVATE ../..)
