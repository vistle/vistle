include_directories(
        ${Boost_INCLUDE_DIRS}
)

add_module(PrintAttributes PrintAttributes.cpp)

target_link_libraries(PrintAttributes
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

