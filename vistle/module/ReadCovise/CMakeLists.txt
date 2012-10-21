add_executable(ReadCovise ReadCovise.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}/core
)

target_link_libraries(ReadCovise
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
