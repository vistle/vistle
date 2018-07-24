include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)

add_module(TestInterpolation TestInterpolation.cpp)

target_link_libraries(TestInterpolation
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

