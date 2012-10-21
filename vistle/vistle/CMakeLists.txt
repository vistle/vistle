include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ../core
        ..
)

add_executable(vistle vistle.cpp)
target_link_libraries(vistle
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
        vistle_control
)

add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
