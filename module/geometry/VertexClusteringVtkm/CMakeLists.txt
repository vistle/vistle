set(HEADER VertexClusteringVtkm.h)
set(SOURCES VertexClusteringVtkm.cpp)

add_vtkm_module(VertexClusteringVtkm "simplify polygonal (i.e. triangle) mesh" ${HEADER} ${SOURCES})
