set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h algo/CalculateThickness.h)
set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp algo/CalculateThickness.cpp)
add_module(SpheresOverlap "create lines between overlapping spheres" ${HEADERS} ${SOURCES})
