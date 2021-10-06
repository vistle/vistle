 set(HEADERS
     GhostCellGenerator.h
 )
 
 set(SOURCES
     GhostCellGenerator.cpp
 )
add_module(GhostCellGenerator "Generate Ghost Cells for UnstructuredGrids" GhostCellGenerator.cpp)
target_include_directories(GhostCellGenerator PRIVATE ${DIY_INCLUDE_PATH})
