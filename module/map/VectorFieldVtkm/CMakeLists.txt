cmake 

set(HEADERS filter/VectorFieldWorklet.h filter/VectorFieldFilter.h VectorFieldVtkm.h)
set(SOURCES filter/VectorFieldFilter.cpp VectorFieldVtkm.cpp)

add_vtkm_module(VectorFieldVtkm "" ${HEADERS} ${SOURCES})

if(VISTLE_USE_CUDA AND VISTLE_USE_SEPARABLE_COMPILATION)
    viskores_add_target_information(vistle_viskores MODIFY_CUDA_FLAGS DEVICE_SOURCES ${SOURCES})
endif()

