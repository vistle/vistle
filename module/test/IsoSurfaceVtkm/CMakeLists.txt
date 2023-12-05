# make sure data types of the externally compiled vtk-m are compatible with vistle
#if(NOT VISTLE_INTERNAL_VTKM)
#    add_subdirectory(compatibility)
#endif()

set(HEADER IsoSurfaceVtkm.h VtkmUtils.h)
set(SOURCES IsoSurfaceVtkm.cpp VtkmUtils.cpp)

# when using Cuda device backend for vtkm, make sure that nvcc
# compiler is used, even if file does not end on .cu
if(VTKm_ENABLE_CUDA)
    set_source_files_properties(${HEADER} ${SOURCES} PROPERTIES LANGUAGE CUDA)
endif()

add_module(IsoSurfaceVtkm "extract surfaces with constant data level" ${HEADER} ${SOURCES})
target_link_libraries(IsoSurfaceVtkm vtkm::cont vtkm::filter)
