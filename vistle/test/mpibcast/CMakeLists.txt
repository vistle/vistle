include_directories(
      ../../../3rdparty/${BOOST_MPI_DIR}/include                                                                                                                                                                                                                                               
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ../..
)

add_executable(mpibcast mpibcast.cpp)
target_link_libraries(mpibcast
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_boost_mpi
)

