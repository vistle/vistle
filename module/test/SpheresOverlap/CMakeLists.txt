set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h algo/VtkmSpheresOverlap.h algo/worklet/PointLocatorCellLists.h algo/worklet/OverlapDetector.h)
set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp algo/VtkmSpheresOverlap.cpp algo/worklet/PointLocatorCellLists.cpp algo/worklet/OverlapDetector.cpp)

add_module(SpheresOverlap "create lines between overlapping spheres" ${HEADER} ${SOURCES})
target_link_libraries(SpheresOverlap vistle_vtkm vtkm::cont vtkm::filter vtkm::worklet cudart)

if (VTKm_ENABLE_CUDA)
    # make sure nvcc compiler is used for the .h and .cpp files
    set_source_files_properties(${HEADER} ${SOURCES} PROPERTIES LANGUAGE CUDA)

    # device code is split into multiple header/source code files, so we need to 
    # enable seperate compilation to avoid linking errors
    set_property(TARGET SpheresOverlap PROPERTY CUDA_SEPARABLE_COMPILATION ON)
endif()
