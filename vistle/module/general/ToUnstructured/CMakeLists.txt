add_module(ToUnstructured ToUnstructured.cpp Cartesian3.h)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
