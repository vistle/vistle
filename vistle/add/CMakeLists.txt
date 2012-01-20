find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

add_executable(add add.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
)

target_link_libraries(add
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
