find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

enable_testing()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}/core
        ${PROJECT_SOURCE_DIR}
)

add_executable(Gendat Gendat.cpp)
target_link_libraries(Gendat
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)

add_executable(GendatChecker GendatChecker.cpp)
target_link_libraries(GendatChecker
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)

add_executable(GendatTest GendatTest.cpp)
add_test(GendatTest ${CMAKE_CURRENT_BINARY_DIR}/GendatTest)
target_link_libraries(GendatTest
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
        vistle_control
)
