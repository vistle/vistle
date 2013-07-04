enable_testing()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

add_executable(Gendat Gendat.cpp)
target_link_libraries(Gendat
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_executable(GendatChecker GendatChecker.cpp)
target_link_libraries(GendatChecker
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)

add_executable(GendatTest GendatTest.cpp)
add_test(GendatTest GendatTest)
target_link_libraries(GendatTest
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
        vistle_control
)
