set(HEADERS SpheresOverlapVtkm.h algo/CalculateThicknessVtkm.h algo/VtkmSpheresOverlap.h algo/worklet/PointLocatorCellLists.h algo/worklet/OverlapDetector.h)

set(SOURCES SpheresOverlapVtkm.cpp DEVICE_SOURCES algo/VtkmSpheresOverlap.cpp)

if(VISTLE_USE_SEPARABLE_COMPILATION)
    # these are also DEVICE_SOURCES
    set(SOURCES ${SOURCES} algo/CalculateThicknessVtkm.cpp algo/worklet/PointLocatorCellLists.cpp algo/worklet/OverlapDetector.cpp)
endif()

add_vtkm_module(SpheresOverlapVtkm "create lines between overlapping spheres" ${HEADERS} ${SOURCES})

if(NOT VISTLE_USE_SEPARABLE_COMPILATION)
    target_compile_definitions(SpheresOverlapVtkm PRIVATE "NONSEPARABLE_COMPILATION")
endif()
