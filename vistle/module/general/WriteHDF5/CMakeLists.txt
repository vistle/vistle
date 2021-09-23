include_directories("/usr/include/mpich-x86_64")
find_package(HDF5 COMPONENTS C)
if(NOT HDF5_IS_PARALLEL)
    message("Parallel HDF5 is required for WriteHDF5")
    return()
endif()

add_definitions(-DH5_HAVE_PARALLEL)

include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})

link_directories(${HDF5_LIBRARY_DIRS})

add_module(WriteHDF5 "write HDF5" WriteHDF5.cpp HDF5Objects.h)

use_openmp()

target_link_libraries(WriteHDF5 ${HDF5_LIBRARIES})
