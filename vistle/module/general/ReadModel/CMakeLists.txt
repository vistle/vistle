vistle_find_package(assimp)
if(NOT assimp_FOUND)
   return()
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASSIMP_LINK_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASSIMP_CXX_FLAGS}")

include_directories(
        ${ASSIMP_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

link_directories(
        ${ASSIMP_LIBRARY_DIRS}
)

add_module(ReadModel ReadModel.cpp)

target_link_libraries(ReadModel
        ${ASSIMP_LIBRARIES}
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
