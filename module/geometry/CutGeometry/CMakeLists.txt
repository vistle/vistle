use_openmp()
add_definitions(-DCUTGEOMETRY)
add_definitions(-DCUTTINGSURFACE)
add_definitions(-DTOGGLESIGN)

add_module(CutGeometry "clip geometry at basic geometry like plane, cylinder or sphere" CutGeometry.cpp PlaneClip.cpp ../../map/IsoSurface/IsoDataFunctor.cpp)
