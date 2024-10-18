find_path(NETCDF_INCLUDE_DIR netcdf.h)
find_path(NETCDF_PAR_INCLUDE_DIR netcdf_par.h)

find_library(NETCDF_LIBRARY NAMES netcdf)
find_library(NETCDF_LIBRARY_DEBUG NAMES netcdfd)

if(MSVC)
    # VisualStudio needs a debug version
    find_library(
        NETCDF_LIBRARY_DEBUG
        NAMES ${NETCDF_DBG_NAMES}
        PATHS $ENV{EXTERNLIBS}/NETCDF
        PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
        DOC "NETCDF - Library (Debug)")

    find_package_handle_standard_args(NETCDF DEFAULT_MSG NETCDF_C_LIBRARY NETCDF_C_LIBRARY_DEBUG NETCDF_INCLUDE_DIR)
    if(NETCDF_FOUND)
        set(NETCDF_LIBRARIES optimized ${NETCDF_LIBRARY} debug ${NETCDF_LIBRARY_DEBUG})
    endif()

    mark_as_advanced(NETCDF_LIBRARY_DEBUG NETCDF_INCLUDE_DIR)
else(MSVC)
    find_package_handle_standard_args(NETCDF DEFAULT_MSG NETCDF_LIBRARY NETCDF_INCLUDE_DIR)
    if(NETCDF_FOUND)
        set(NETCDF_LIBRARIES ${NETCDF_LIBRARY})
    endif()
endif(MSVC)

if(NETCDF_FOUND)
    set(NETCDF_INCLUDE_DIRS ${NETCDF_INCLUDE_DIR})
    if(NETCDF_INCLUDE_DIR STREQUAL NETCDF_PAR_INCLUDE_DIR)
        set(NETCDF_PARALLEL TRUE)
    endif()
endif()
