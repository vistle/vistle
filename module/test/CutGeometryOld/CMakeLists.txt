use_openmp()
add_definitions(-DCUTGEOMETRYOLD)
add_definitions(-DCUTGEOMETRY)
add_definitions(-DCUTTINGSURFACE)
add_definitions(-DTOGGLESIGN)

add_module(CutGeometryOld "clip geometry at basic geometry like plane, cylinder or sphere" CutGeometry.cpp ../../map/IsoSurface/IsoDataFunctor.cpp)
