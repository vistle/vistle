add_module(ReadVistle ReadVistle.cpp)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(ReadVistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
