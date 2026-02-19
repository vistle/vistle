set(HEADERS filter/DisplaceWorklet.h filter/DisplaceFilter.h DisplaceVtkm.h)
set(SOURCES DisplaceVtkm.cpp DEVICE_SOURCES filter/DisplaceFilter.cpp)

add_vtkm_module(DisplaceVtkm "move vertices according to mapped data of first input" ${HEADERS} DEVICE_SOURCES ${SOURCES})
