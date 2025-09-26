add_module(CutGeometry "clip geometry at basic geometry like plane, cylinder or sphere" CutGeometry.cpp PlaneClip.cpp ../../map/IsoSurface/IsoDataFunctor.cpp)

target_compile_definitions(CutGeometry PRIVATE -DCUTGEOMETRY -DCUTTINGSURFACE -DTOGGLESIGN)
