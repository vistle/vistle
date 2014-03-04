include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ../..
)

add_executable(shmtest shmtest.cpp)
target_link_libraries(shmtest
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
)
