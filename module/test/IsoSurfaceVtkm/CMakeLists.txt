# unfortunately, vistle_find_package(VTKm 2.0) does not accept VTK-m 2.1
#vistle_find_package(VTKm 2.1)
#if(NOT VTKm_FOUND)
#    vistle_find_package(VTKm 2.0)
#    if(NOT VTKm_FOUND)
#        return()
#    endif()
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
#target_link_libraries(IsoSurfaceVtkm vtkm_cont vtkm_filter)
