add_module(CuttingSurface CuttingSurface.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(CuttingSurface
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
