include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ../..
)

add_executable(vistle_shminfo vistle_shminfo.cpp)
target_link_libraries(vistle_shminfo
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
)
