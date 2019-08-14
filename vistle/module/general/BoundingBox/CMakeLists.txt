add_definitions(-DBOUNDINGBOX)

add_module(BoundingBox ../Extrema/Extrema.cpp)

use_openmp()
