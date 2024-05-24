set(HEADER ClipVtkm.h)
set(SOURCES ClipVtkm.cpp)

add_vtkm_module(ClipVtkm "clip geometry at surface (CutGeometry)" ${HEADER} ${SOURCES})
