#vistle_find_package(Boost 1.53 REQUIRED COMPONENTS mpi)

add_module(MpiInfo MpiInfo.cpp)

use_openmp()

include_directories(
        ../../../3rdparty/${BOOST_MPI_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(MpiInfo
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_module
        vistle_boost_mpi
)
