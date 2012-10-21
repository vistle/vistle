add_library(vistle_control SHARED portmanager.cpp communicator.cpp executor.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
)

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
