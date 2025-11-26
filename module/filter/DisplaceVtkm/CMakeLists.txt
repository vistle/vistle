set(HEADERS filter/DisplaceWorklet.h filter/DisplaceFilter.h DisplaceVtkm.h)
set(SOURCES filter/DisplaceFilter.cpp DisplaceVtkm.cpp)

add_vtkm_module(DisplaceVtkm "move vertices according to mapped data of first input" ${HEADERS} ${SOURCES})

if(VISTLE_USE_CUDA AND VISTLE_USE_SEPARABLE_COMPILATION)
    viskores_add_target_information(vistle_viskores MODIFY_CUDA_FLAGS DEVICE_SOURCES ${SOURCES})
endif()
