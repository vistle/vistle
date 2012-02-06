find_package(MPI REQUIRED)
find_package(Boost 1.42 REQUIRED COMPONENTS date_time iostreams)

add_executable(ReadFOAM ReadFOAM.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
)

target_link_libraries(ReadFOAM
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
