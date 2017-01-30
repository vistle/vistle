set(NAME IsoSurface)
include(IsoSurface.cmake)
add_custom_target(IsoSurfaces_CUDA SOURCES ../IsoSurface/Leveller.cu)
