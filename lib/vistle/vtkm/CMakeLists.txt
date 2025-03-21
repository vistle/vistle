set(SOURCES convert.cpp convert_topology.cpp convert_worklets.cpp module_status.cpp implicit_function_controller.cpp vtkm_module.cpp)

set(HEADERS
    export.h
    convert.h
    convert_topology.h
    convert_worklets.h
    module_status.h
    implicit_function_controller.h
    vtkm_module.h)

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
    vtkm::cont
    vtkm::filter)
