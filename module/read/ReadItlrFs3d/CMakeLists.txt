vistle_find_package(HDF5 COMPONENTS C)

#if (NOT HDF5_IS_PARALLEL)
#    message("Parallel HDF5 is required for ReadHDF5")
#    return()
#endif()
#
#add_definitions(-DH5_HAVE_PARALLEL)

if(HDF5_FOUND)
    include_directories(SYSTEM ${HDF5_INCLUDE_DIRS})

    link_directories(${HDF5_LIBRARY_DIRS})

    add_definitions(-DHAVE_HDF5)
endif()

add_module(ReadItlrFs3d "read ITLR FS3D (Free Surface 3D) binary data" ReadItlrBin.cpp)
if(HDF5_FOUND)
    target_link_libraries(ReadItlrFs3d ${HDF5_LIBRARIES})
endif()
