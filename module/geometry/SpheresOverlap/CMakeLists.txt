set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h algo/ThicknessDeterminer.h algo/VtkmSpheresOverlap.h algo/worklet/PointLocatorCellLists.h
            algo/worklet/OverlapDetector.h)

set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp DEVICE_SOURCES algo/VtkmSpheresOverlap.cpp)

if(VISTLE_USE_SEPARABLE_COMPILATION)
    set(SOURCES ${SOURCES} algo/ThicknessDeterminer.cpp algo/worklet/PointLocatorCellLists.cpp algo/worklet/OverlapDetector.cpp)
endif()

add_vtkm_module(SpheresOverlap "create lines between overlapping spheres" ${HEADER} ${SOURCES})
target_link_libraries(SpheresOverlap PRIVATE viskores::worklet)

if(NOT VISTLE_USE_SEPARABLE_COMPILATION)
    target_compile_definitions(SpheresOverlap PRIVATE "NONSEPARABLE_COMPILATION")
endif()
