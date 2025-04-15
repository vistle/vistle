# - Try to find PNetCDF
# Define:
# PNETCDF_FOUND - System has PNetCDF
# PNETCDF_INCLUDE_DIRS - PNetCDF include directories
# PNETCDF_LIBRARIES - libraries needed for PNetCDF
# PNETCDF_DEFINITIONS - Compiler switches for using PNetCDF

if(NOT VISTLE_USE_MPI)
    set(PNETCDF_FOUND FALSE)
else()

    find_package(PkgConfig)
    pkg_check_modules(PC_PNETCDF QUIET pnetcdf)
    set(PNETCDF_DEFINITIONS ${PC_PNETCDF_CFLAGS_OTHER})

    find_path(
        PNETCDF_INCLUDE_DIR pnetcdf pnetcdf.h
        HINTS ${PC_PNETCDF_INCLUDEDIR} ${PC_PNETCDF_INCLUDE_DIRS}
        PATH_SUFFIXES pnetcdf)

    find_library(
        PNETCDF_LIBRARY
        NAMES pnetcdf libpnetcdf
        HINTS ${PC_PNETCDF_LIBDIR} ${PC_PNETCDF_LIBRARY_DIRS})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(PNETCDF DEFAULT_MSG PNETCDF_LIBRARY PNETCDF_INCLUDE_DIR)

    mark_as_advanced(PNETCDF_INCLUDE_DIR PNETCDF_LIBRARY)

    if(PNETCDF_FOUND)
        set(PNETCDF_LIBRARIES ${PNETCDF_LIBRARY})
        set(PNETCDF_INCLUDE_DIRS ${PNETCDF_INCLUDE_DIR})
    endif()

endif()
