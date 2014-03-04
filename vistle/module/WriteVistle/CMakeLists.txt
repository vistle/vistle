find_package(Boost 1.40 REQUIRED COMPONENTS serialization)

add_module(WriteVistle WriteVistle.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(WriteVistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
