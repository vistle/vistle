add_executable(libsimbcast mpibcast.cpp)
target_link_libraries(
    libsimbcast
    PRIVATE Boost::boost
    PRIVATE MPI::MPI_C
    PRIVATE ${BOOST_MPI}
    PRIVATE vistle_insitu_core)

target_include_directories(libsimbcast PRIVATE ../..)
