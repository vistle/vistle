#find_package(Boost 1.40 REQUIRED COMPONENTS mpi)

add_executable(Extrema Extrema.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ../../../3rdparty/boost-mpi/include
)

target_link_libraries(Extrema
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
