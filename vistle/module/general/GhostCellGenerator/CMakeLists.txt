vistle_find_package(VTK
    COMPONENTS vtkFiltersParallelDIY2 NO_MODULE)
set(HEADERS
    GhostCellGenerator.h
)

set(SOURCES
    GhostCellGenerator.cpp
)

if (NOT VTK_FOUND)
    return()
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-inconsistent-missing-override")
endif()

add_module(GhostCellGenerator "Generate Ghost Cells for UnstructuredGrids" GhostCellGenerator.cpp)

target_include_directories(GhostCellGenerator 
    PRIVATE ${DIY_INCLUDE_PATH} 
    PRIVATE ${VTK_INCLUDE_DIRS})

target_link_libraries(GhostCellGenerator
   ${VTK_LIBRARIES}
   vistle_vtk
)
