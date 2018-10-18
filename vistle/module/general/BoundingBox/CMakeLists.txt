#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS mpi)
add_definitions(-DBOUNDINGBOX)

add_module(BoundingBox ../Extrema/Extrema.cpp)

use_openmp()
