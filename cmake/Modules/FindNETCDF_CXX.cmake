find_package(NETCDF)

if(NETCDF_FOUND)
    find_path(NETCDF_CXX_INCLUDE_DIR netcdf)

    find_library(NETCDF_CXX_LIBRARY NAMES netcdf-cxx4 netcdf_c++4)
    find_library(NETCDF_CXX_LIBRARY_DEBUG NAMES netcdf-cxx4d netcdf_c++4d)

    if(MSVC)
        # VisualStudio needs a debug version
        find_library(
            NETCDF_CXX_LIBRARY_DEBUG
            NAMES ${NETCDF_CXX_DBG_NAMES}
            PATHS $ENV{EXTERNLIBS}/NETCDF
            PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
            DOC "NETCDF_CXX - Library (Debug)")

        find_package_handle_standard_args(NETCDF_CXX DEFAULT_MSG NETCDF_CXX_LIBRARY NETCDF_CXX_LIBRARY_DEBUG NETCDF_INCLUDE_DIR NETCDF_CXX_INCLUDE_DIR)
        if(NETCDF_CXX_FOUND)
            set(NETCDF_CXX_LIBRARIES ${NETCDF_LIBRARIES} optimized ${NETCDF_CXX_LIBRARY} debug ${NETCDF_CXX_LIBRARY_DEBUG})
        endif()

        mark_as_advanced(NETCDF_CXX_LIBRARY NETCDF_CXX_LIBRARY_DEBUG NETCDF_CXX_INCLUDE_DIR)
    else(MSVC)
        find_package_handle_standard_args(NETCDF_CXX DEFAULT_MSG NETCDF_CXX_LIBRARY NETCDF_CXX_INCLUDE_DIR)
        if(NETCDF_CXX_FOUND)
            set(NETCDF_CXX_LIBRARIES ${NETCDF_LIBRARIES} ${NETCDF_CXX_LIBRARY})
        endif()
    endif(MSVC)

    if(NETCDF_CXX_FOUND)
        set(NETCDF_CXX_INCLUDE_DIRS ${NETCDF_INCLUDE_DIRS} ${NETCDF_CXX_INCLUDE_DIR})
    endif()
endif(NETCDF_FOUND)
