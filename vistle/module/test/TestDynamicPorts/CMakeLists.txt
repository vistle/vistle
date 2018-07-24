include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)

add_module(TestDynamicPorts TestDynamicPorts.cpp)

target_link_libraries(TestDynamicPorts
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

