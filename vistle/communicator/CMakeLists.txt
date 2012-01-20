find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

add_executable(communicator communicator_collective.cpp portmanager.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
)

target_link_libraries(communicator
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)