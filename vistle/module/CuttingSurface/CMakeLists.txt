find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

add_executable(CuttingSurface CuttingSurface.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}/core
)

target_link_libraries(CuttingSurface
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
