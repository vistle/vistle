set(HEADER IsoSurfaceVtkm.h)
set(SOURCES IsoSurfaceVtkm.cpp)

# when using Cuda device backend for vtkm, make sure that nvcc
# compiler is used, even if file does not end on .cu
if(VTKm_ENABLE_CUDA)
    set_source_files_properties(${HEADER} ${SOURCES} PROPERTIES LANGUAGE CUDA)
endif()

add_module(IsoSurfaceVtkm "extract surfaces with constant data level" ${HEADER} ${SOURCES})
target_link_libraries(IsoSurfaceVtkm vistle_vtkm vtkm::cont vtkm::filter)
