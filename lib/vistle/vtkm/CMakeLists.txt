set(SOURCES cellsetToVtkm.cpp convert.cpp convert_worklets.cpp)

set(HEADERS export.h transformEnums.h cellsetToVtkm.h convert.h convert_worklets.h)

vistle_add_library(vistle_vtkm EXPORT ${SOURCES} ${HEADERS})
target_include_directories(vistle_vtkm SYSTEM PUBLIC ${VTKM_INCLUDE_DIRS})

if(VISTLE_CUDA_ENABLED)
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CUDA)
endif()

vistle_target_link_libraries(
    vistle_vtkm
    PUBLIC
    ${VTK_LIBRARIES}
    PUBLIC
    vistle_core
    vtkm::cont
    vtkm::filter)
