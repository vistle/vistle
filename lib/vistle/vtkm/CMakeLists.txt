set(SOURCES convert.cpp convert_topology.cpp convert_worklets.cpp)

set(HEADERS export.h convert.h convert_topology.h convert_worklets.h)

vistle_add_library(vistle_vtkm EXPORT ${SOURCES} ${HEADERS})
target_include_directories(vistle_vtkm SYSTEM PUBLIC ${VTKM_INCLUDE_DIRS})
vtkm_add_target_information(vistle_vtkm DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS ${SOURCES} ${HEADERS}) # assume that everything is device code

vistle_target_link_libraries(vistle_vtkm PUBLIC ${VTK_LIBRARIES} PRIVATE vistle_core vtkm::cont)
