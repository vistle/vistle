enable_testing()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

add_module(GenIsoDat GenIsoDat.cpp)
target_link_libraries(GenIsoDat
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)
