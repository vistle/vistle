add_module(ColorAttribute ColorAttribute.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ColorAttribute
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
