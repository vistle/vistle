include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../..
)

add_executable(shmtest shmtest.cpp)
target_link_libraries(shmtest
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
