add_module(PrintMetaData PrintMetaData.cpp)

use_openmp()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

target_link_libraries(PrintMetaData
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
)
