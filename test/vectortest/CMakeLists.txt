add_executable(vectortest vectortest.cpp)
target_link_libraries(
    vectortest
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_C
    PRIVATE vistle_core)
target_include_directories(vectortest PRIVATE ../..)
