#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS mpi)
add_definitions(-DBOUNDINGBOX)

add_module(BoundingBox ../Extrema/Extrema.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(BoundingBox
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
