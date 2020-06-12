include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ../..
)

add_executable(vistle_shmtest shmtest.cpp)
target_link_libraries(vistle_shmtest
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        Threads::Threads
)
