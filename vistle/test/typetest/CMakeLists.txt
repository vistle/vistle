include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ../..
)

add_executable(typetest typetest.cpp)
target_link_libraries(typetest
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        Threads::Threads
)
