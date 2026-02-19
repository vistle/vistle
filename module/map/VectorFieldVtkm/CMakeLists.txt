set(HEADERS filter/VectorFieldWorklet.h filter/VectorFieldFilter.h VectorFieldVtkm.h)
set(SOURCES filter/VectorFieldFilter.cpp VectorFieldVtkm.cpp)

add_vtkm_module(VectorFieldVtkm "create lines from mapped vector data" ${HEADERS} DEVICE_SOURCES ${SOURCES})
