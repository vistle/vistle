add_module(ToTriangles ToTriangles.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ToTriangles
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
