set(HEADER CellClipVtkm.h)
set(SOURCES CellClipVtkm.cpp)

add_vtkm_module(CellClipVtkm "clip geometry at surface (CutGeometry)" ${HEADER} ${SOURCES})
