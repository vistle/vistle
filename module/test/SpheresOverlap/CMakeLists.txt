set(HEADERS SpheresOverlap.h algo/CellListsAlgorithm.h)
set(SOURCES SpheresOverlap.cpp algo/CellListsAlgorithm.cpp)

add_module(SpheresOverlap "create lines between overlapping spheres" ${HEADER} ${SOURCES})
