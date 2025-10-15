set(HEADER PassThroughVtkm.h)
set(SOURCES PassThroughVtkm.cpp)

add_vtkm_module(PassThroughVtkm "convert data to Viskores/VTK-m and back, unmodified" ${HEADER} ${SOURCES})
