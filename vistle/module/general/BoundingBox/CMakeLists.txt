add_definitions(-DBOUNDINGBOX)

add_module(BoundingBox "compute extremal values" ../Extrema/Extrema.cpp)

use_openmp()
