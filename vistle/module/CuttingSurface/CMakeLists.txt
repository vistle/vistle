add_executable(CuttingSurface CuttingSurface.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(CuttingSurface
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_core
)
