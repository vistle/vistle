set(NAME CuttingSurface)
set(DESCRIPTION "cut through grids with basic geometry like plane, cylinder or sphere, extracting a lower-dimensional slice or surface")
add_definitions(-DCUTTINGSURFACE)
include("../IsoSurface/IsoSurface.cmake")
