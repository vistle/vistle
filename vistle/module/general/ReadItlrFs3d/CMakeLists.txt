add_module(ReadItlrFs3d ReadItlrBin.cpp)
target_link_libraries(ReadItlrFs3d
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
