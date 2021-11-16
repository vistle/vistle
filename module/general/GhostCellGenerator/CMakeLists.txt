if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-inconsistent-missing-override")
endif()

set(HEADERS GhostCellGenerator.h)

set(SOURCES GhostCellGenerator.cpp)
add_module(GhostCellGenerator "Generate Ghost Cells for UnstructuredGrids" GhostCellGenerator.cpp)

target_include_directories(GhostCellGenerator PRIVATE ${DIY_INCLUDE_PATH})

#diy
# target_include_directories(vistle_module PUBLIC $<BUILD_INTERFACE:${DIY_INCLUDE_DIR}>)
