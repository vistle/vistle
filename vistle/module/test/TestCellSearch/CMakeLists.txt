include_directories(
        ${Boost_INCLUDE_DIRS}
)

add_module(TestCellSearch TestCellSearch.cpp)

target_link_libraries(TestCellSearch
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

