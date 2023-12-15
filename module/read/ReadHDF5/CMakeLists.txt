include_directories("/usr/include/mpich-x86_64")
vistle_find_package(HDF5 COMPONENTS C)
if(NOT HDF5_IS_PARALLEL)
    message("Parallel HDF5 is required for ReadHDF5")
    return()
endif()

add_definitions(-DH5_HAVE_PARALLEL)

include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})

link_directories(${HDF5_LIBRARY_DIRS})

use_openmp()
add_module(ReadHDF5 "read Vistle data stored in HDF5 format" ReadHDF5.cpp)

target_link_libraries(ReadHDF5 ${HDF5_LIBRARIES})
