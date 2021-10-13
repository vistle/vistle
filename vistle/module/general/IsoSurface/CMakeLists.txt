set(NAME IsoSurface)
set(DESCRIPTION "extract surfaces with constant data level")
include(IsoSurface.cmake)
add_custom_target(IsoSurfaces_CUDA SOURCES ../IsoSurface/Leveller.cu)
