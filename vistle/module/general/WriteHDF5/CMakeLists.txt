include_directories("/usr/include/mpich-x86_64")
FIND_PACKAGE(HDF5 COMPONENTS C)
if (NOT HDF5_IS_PARALLEL)
    message("Parallel HDF5 is required for WriteHDF5")
    return()
endif()

add_definitions(-DH5_HAVE_PARALLEL)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
	${HDF5_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}
)

link_directories(
        ${HDF5_LIBRARY_DIRS}
)

add_module(WriteHDF5 WriteHDF5.cpp vistleObject_oarchive.h)

use_openmp()

target_link_libraries(WriteHDF5
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
	${HDF5_LIBRARIES}
        vistle_module
)
