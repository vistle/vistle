add_executable(CutGeometry CutGeometry.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}/core
)

target_link_libraries(CutGeometry
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
