add_module(Color Color.cpp)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(Color
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
