set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h algo/VtkmSpheresOverlap.h algo/PointLocatorCellLists.h)
set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp algo/VtkmSpheresOverlap.cpp algo/PointLocatorCellLists.cpp)

add_module(SpheresOverlap "create lines between overlapping spheres" ${HEADER} ${SOURCES})
target_link_libraries(SpheresOverlap vistle_vtkm vtkm::cont vtkm::filter vtkm::worklet)
