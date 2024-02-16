set(HEADER ClipVtkm.h)
set(SOURCES ClipVtkm.cpp)

# when using Cuda device backend for vtkm, make sure that nvcc
# compiler is used, even if file does not end on .cu
if(VTKm_ENABLE_CUDA)
    set_source_files_properties(${HEADER} ${SOURCES} PROPERTIES LANGUAGE CUDA)
endif()

add_module(ClipVtkm "clip geometry at surface (CutGeometry)" ${HEADER} ${SOURCES})
target_link_libraries(ClipVtkm vistle_vtkm vtkm::cont vtkm::filter)
