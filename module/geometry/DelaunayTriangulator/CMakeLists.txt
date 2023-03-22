add_library(vistle_tetgen STATIC tetgen/tetgen.cxx tetgen/predicates.cxx)
set_target_properties(vistle_tetgen PROPERTIES "COMPILE_DEFINITIONS" TETLIBRARY)
if(VISTLE_MODULES_SHARED)
    set_target_properties(vistle_tetgen PROPERTIES "POSITION_INDEPENDENT_CODE" TRUE)
endif()

add_module(DelaunayTriangulator "combine points to structured grid based on 3D Delaunay Triangulation" DelaunayTriangulator.cpp DelaunayTriangulator.h)
target_link_libraries(DelaunayTriangulator vistle_tetgen)
target_include_directories(DelaunayTriangulator PRIVATE tetgen)
