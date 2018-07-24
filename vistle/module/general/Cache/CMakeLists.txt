add_module(Cache Cache.cpp)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(Cache
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
