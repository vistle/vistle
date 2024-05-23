set(SOURCES convert.cpp)

set(HEADERS export.h convert.h)

vistle_add_library(vistle_vtkm EXPORT ${SOURCES} ${HEADERS})
target_include_directories(vistle_vtkm SYSTEM PUBLIC ${VTKM_INCLUDE_DIRS})

if(VTKm_ENABLE_CUDA)
    set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE CUDA)
endif()

vistle_target_link_libraries(vistle_vtkm PUBLIC ${VTK_LIBRARIES} PUBLIC vistle_core vtkm::cont vtkm::filter)
