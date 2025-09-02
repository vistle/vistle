vistle_find_package(TINYXML2)
if(TINYXML2_FOUND)
    add_definitions(-DHAVE_TINYXML2)
    include_directories(SYSTEM ${TINYXML2_INCLUDE_DIRS})
endif()

if(NOT VTK_FOUND)
    return()
endif()

set_property(
    DIRECTORY
    APPEND
    PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-inconsistent-missing-override")
endif()

add_module(ReadVtk "read VTK data" ReadVtk.cpp ReadVtk.h)

include_directories(SYSTEM ${VTK_INCLUDE_DIRS})

target_link_libraries(ReadVtk PRIVATE ${VTK_LIBRARIES} vistle_vtk)

if(TINYXML2_FOUND)
    target_link_libraries(ReadVtk PRIVATE ${TINYXML2_LIBRARIES})
endif()
