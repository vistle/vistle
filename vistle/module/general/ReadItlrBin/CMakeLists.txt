add_module(ReadItlrBin ReadItlrBin.cpp)
target_link_libraries(ReadItlrBin
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
