add_definitions(-DBOUNDINGBOX)

use_openmp()
add_module(BoundingBox "compute bounding boxes" ../Extrema/Extrema.cpp)
