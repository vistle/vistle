add_module(Variant Variant.cpp)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(Variant
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
