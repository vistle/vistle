add_executable(vistle_typetest typetest.cpp)
target_link_libraries(
    vistle_typetest
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_C
    PRIVATE vistle_core
    PRIVATE Threads::Threads)

target_include_directories(vistle_typetest PRIVATE ../..)
