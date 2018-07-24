add_module(ToUnstructured ToUnstructured.cpp Cartesian3.h)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(ToUnstructured
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
