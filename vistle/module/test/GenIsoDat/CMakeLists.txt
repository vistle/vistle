enable_testing()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
)

add_module(GenIsoDat GenIsoDat.cpp)
target_link_libraries(GenIsoDat
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)
