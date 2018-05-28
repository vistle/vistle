set(CMAKE_MODULE_PATH "$ENV{COVISEDIR}/cmake" ${CMAKE_MODULE_PATH})
vistle_find_package(COVISE COMPONENTS File)
if(NOT COVISE_FILE_FOUND)
   return()
endif()

add_definitions(-DREAD_DIRECTORY)

add_module(ReadCoviseDirectory ../ReadCovise/ReadCovise.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_FILE_INCLUDE_DIRS}
)

target_link_libraries(ReadCoviseDirectory
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${COVISE_FILE_LIBRARY}
        vistle_module
)
