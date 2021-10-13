vistle_find_package(assimp)
if(NOT assimp_FOUND)
    return()
endif()

include_directories(SYSTEM ${ASSIMP_INCLUDE_DIRS})

link_directories(${ASSIMP_LIBRARY_DIRS})

add_module(ReadModel "read polygonal geometry models with Assimp (STL, OBJ, 3DS, Collada, DXF, PLY, X3D, ...)" ReadModel.cpp)

target_link_libraries(ReadModel ${ASSIMP_LIBRARIES})
