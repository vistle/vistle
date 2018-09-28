set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
vistle_find_package(COVISE COMPONENTS File)
if(NOT COVISE_FILE_FOUND)
   return()
endif()

add_definitions(-DREAD_DIRECTORY)

add_module(ReadCoviseDirectory ../ReadCovise/ReadCovise.cpp)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ${COVISE_FILE_INCLUDE_DIRS}
)

target_link_libraries(ReadCoviseDirectory
        ${COVISE_FILE_LIBRARY}
)
