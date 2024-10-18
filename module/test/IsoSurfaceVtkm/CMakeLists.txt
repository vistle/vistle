set(HEADER IsoSurfaceVtkm.h)
set(SOURCES IsoSurfaceVtkm.cpp)

add_vtkm_module(IsoSurfaceVtkm "extract surfaces with constant data level" ${HEADER} ${SOURCES})
