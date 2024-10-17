add_executable(mpibcast mpibcast.cpp)
target_link_libraries(
    mpibcast
    PRIVATE Boost::boost
    PRIVATE MPI::MPI_CXX
    PRIVATE ${BOOST_MPI})

target_include_directories(mpibcast PRIVATE ../..)
