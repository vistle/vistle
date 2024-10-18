add_executable(vistle_shminfo vistle_shminfo.cpp)
target_link_libraries(vistle_shminfo Boost::boost MPI::MPI_CXX vistle_util_mpi vistle_core vistle_util)

target_include_directories(vistle_shminfo PRIVATE ../..)
