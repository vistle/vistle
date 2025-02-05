set(SOURCES convert.cpp convert_topology.cpp convert_worklets.cpp ImplFuncController.cpp)

set(HEADERS export.h convert.h convert_topology.h convert_worklets.h ImplFuncController.h)

vistle_add_library(vistle_vtkm EXPORT ${SOURCES} ${HEADERS})
target_include_directories(vistle_vtkm SYSTEM PUBLIC ${VTKM_INCLUDE_DIRS})
vtkm_add_target_information(vistle_vtkm MODIFY_CUDA_FLAGS DEVICE_SOURCES convert_worklets.cpp)

vistle_target_link_libraries(
    vistle_vtkm
    PUBLIC
    ${VTK_LIBRARIES}
    PRIVATE
    vistle_core
    vistle_module
    vtkm::cont)
