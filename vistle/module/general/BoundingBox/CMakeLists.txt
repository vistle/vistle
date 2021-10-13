add_definitions(-DBOUNDINGBOX)

add_module(BoundingBox "compute bounding boxes" ../Extrema/Extrema.cpp)

use_openmp()
