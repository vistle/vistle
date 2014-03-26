#find_package(Boost 1.40 REQUIRED COMPONENTS mpi)

add_executable(CreateCelltree CreateCelltree.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(CreateCelltree
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
