include_directories(SYSTEM
        ${MPI_C_INCLUDE_PATH}
)

include_directories(../..)

add_executable(vistle_mpitest vistle_mpitest.cpp)
target_link_libraries(vistle_mpitest ${MPI_C_LIBRARIES} vistle_util)
