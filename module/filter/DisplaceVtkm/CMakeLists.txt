set(HEADERS filter/DisplaceWorklet.h filter/DisplaceFilter.h DisplaceVtkm.h)
set(SOURCES filter/DisplaceFilter.cpp DisplaceVtkm.cpp)

add_vtkm_module(DisplaceVtkm "move vertices according to mapped data of first input" ${HEADERS} ${SOURCES})
