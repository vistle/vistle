add_executable(Add Add.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Add
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
