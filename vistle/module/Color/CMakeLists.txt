add_module(Color Color.cpp)

use_openmp()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(Color
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        vistle_module
)
