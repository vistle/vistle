include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
        ..
)

add_executable(shmtest shmtest.cpp)
target_link_libraries(shmtest
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)

add_executable(vectortest vectortest.cpp)
target_link_libraries(vectortest
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
