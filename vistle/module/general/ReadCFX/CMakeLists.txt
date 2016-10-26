include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

add_module(ReadCFX ReadCFX.cpp)

target_link_libraries(ReadCFX
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
