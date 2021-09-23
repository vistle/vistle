add_executable(typetest typetest.cpp)
target_link_libraries(
    typetest
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_C
    PRIVATE vistle_core
    PRIVATE Threads::Threads)

target_include_directories(typetest PRIVATE ../..)
