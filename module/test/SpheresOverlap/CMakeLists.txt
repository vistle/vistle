set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h algo/VtkmSpheresOverlap.h algo/worklet/PointLocatorCellLists.h algo/worklet/OverlapDetector.h)
set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp algo/VtkmSpheresOverlap.cpp algo/worklet/PointLocatorCellLists.cpp algo/worklet/OverlapDetector.cpp)

add_module(SpheresOverlap "create lines between overlapping spheres" ${HEADER} ${SOURCES})
target_link_libraries(SpheresOverlap vistle_vtkm vtkm::cont vtkm::filter vtkm::worklet)
