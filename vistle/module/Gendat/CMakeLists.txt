enable_testing()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

add_module(Gendat Gendat.cpp)
target_link_libraries(Gendat
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_module(GendatChecker GendatChecker.cpp)
target_link_libraries(GendatChecker
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
