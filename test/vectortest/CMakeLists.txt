add_executable(vectortest vectortest.cpp)
target_link_libraries(
    vectortest
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_CXX
    PRIVATE vistle_core
    PRIVATE vistle_util)
target_include_directories(vectortest PRIVATE ../..)
