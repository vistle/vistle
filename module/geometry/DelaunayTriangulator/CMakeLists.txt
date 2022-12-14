find_package(TetGen)
if(NOT ${TetGen_FOUND})
    return()
endif()

add_module(DelaunayTriangulator "combine points to structured grid based on 3D Delaunay Triangulation" DelaunayTriangulator.cpp DelaunayTriangulator.h)
target_link_libraries(DelaunayTriangulator ${TetGen_LIBRARY})
target_include_directories(DelaunayTriangulator PRIVATE ${TetGen_INCLUDE_DIR})
