# - Find hwloc
# Find the Portable Hardware Locality (hwloc)
#
# This module defines the following variables:
#   HWLOC_FOUND  -  True if library and include directory are found
# If set to TRUE, the following are also defined:
#   HWLOC_INCLUDE_DIRS  -  The directory where to find the header file
#   HWLOC_LIBRARIES  -  Where to find the library files
#
# This file is in the public domain

include(FindPackageHandleStandardArgs)

set(HWLOC_NAMES hwloc)
set(HWLOC_NAMES_DEBUG hwlocd)

find_path(
    HWLOC_INCLUDE_DIR
    NAMES hwloc.h
    PATH_SUFFIXES ${HWLOC_NAMES}
    DOC "The hwloc include directory")

find_library(
    HWLOC_LIBRARY
    NAMES ${HWLOC_NAMES}
    DOC "The hwloc (release) library")
if(MSVC)
    find_library(
        HWLOC_LIBRARY_DEBUG
        NAMES ${HWLOC_NAMES_DEBUG}
        DOC "The hwloc debug library")
    find_package_handle_standard_args(HWLOC REQUIRED_VARS HWLOC_LIBRARY HWLOC_LIBRARY_DEBUG HWLOC_INCLUDE_DIR)
else()
    find_package_handle_standard_args(HWLOC REQUIRED_VARS HWLOC_LIBRARY HWLOC_INCLUDE_DIR)
endif()

if(HWLOC_FOUND)
    set(HWLOC_INCLUDE_DIRS ${HWLOC_INCLUDE_DIR})
    if(MSVC)
        set(HWLOC_LIBRARIES optimized ${HWLOC_LIBRARY} debug ${HWLOC_LIBRARY_DEBUG})
    else()
        set(HWLOC_LIBRARIES ${HWLOC_LIBRARY})
    endif()
endif()

mark_as_advanced(HWLOC_INCLUDE_DIR HWLOC_LIBRARY HWLOC_LIBRARY_DEBUG)
