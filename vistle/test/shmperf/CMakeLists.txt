include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../..
)

add_executable(shmperf vectortest.cpp)
target_link_libraries(shmperf
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
