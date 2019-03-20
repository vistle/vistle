vistle_find_package(assimp)
if(NOT assimp_FOUND)
   return()
endif()

include_directories(
        ${ASSIMP_INCLUDE_DIRS}
)

link_directories(
        ${ASSIMP_LIBRARY_DIRS}
)

add_module(ReadModel ReadModel.cpp)

target_link_libraries(ReadModel
        ${ASSIMP_LIBRARIES}
)
