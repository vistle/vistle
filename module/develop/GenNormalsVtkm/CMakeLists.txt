set(HEADER GenNormalsVtkm.h)
set(SOURCES GenNormalsVtkm.cpp)

add_vtkm_module(GenNormalsVtkm "compute surface normals" ${HEADER} ${SOURCES})
