find_package(COVISE)
if(NOT COVISE_FOUND)
   return()
endif()

add_executable(ReadCovise ReadCovise.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${COVISE_INCLUDE_DIRS}
)

target_link_libraries(ReadCovise
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${COVISE_FILE_LIBRARY}
        vistle_module
)
