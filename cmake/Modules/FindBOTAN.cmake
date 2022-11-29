# - Find botan
# Find the botan cryptographic library
#
# This module defines the following variables:
#   BOTAN_FOUND  -  True if library and include directory are found
# If set to TRUE, the following are also defined:
#   BOTAN_INCLUDE_DIRS  -  The directory where to find the header file
#   BOTAN_LIBRARIES  -  Where to find the library files
#
# This file is in the public domain

include(FindPackageHandleStandardArgs)

set(BOTAN_NAMES botan botan-2 botan-1)
set(BOTAN_NAMES_DEBUG botand botan-2d botan-1d)

find_path(
    BOTAN_INCLUDE_DIR
    NAMES botan/botan.h botan-3/botan/botan.h
    PATH_SUFFIXES ${BOTAN_NAMES}
    DOC "The Botan include directory")

find_library(
    BOTAN_LIBRARY
    NAMES ${BOTAN_NAMES}
    PATH_SUFFIXES release/lib
    DOC "The Botan (release) library")
if(MSVC)
    find_library(
        BOTAN_LIBRARY_DEBUG
        NAMES ${BOTAN_NAMES_DEBUG}
        PATH_SUFFIXES debug/lib
        DOC "The Botan debug library")
    find_package_handle_standard_args(BOTAN REQUIRED_VARS BOTAN_LIBRARY BOTAN_LIBRARY_DEBUG BOTAN_INCLUDE_DIR)
else()
    find_package_handle_standard_args(BOTAN REQUIRED_VARS BOTAN_LIBRARY BOTAN_INCLUDE_DIR)
endif()

if(BOTAN_FOUND)
    set(BOTAN_INCLUDE_DIRS ${BOTAN_INCLUDE_DIR})
    if(MSVC)
        set(BOTAN_LIBRARIES optimized ${BOTAN_LIBRARY} debug ${BOTAN_LIBRARY_DEBUG})
    else()
        set(BOTAN_LIBRARIES ${BOTAN_LIBRARY})
    endif()
endif()

mark_as_advanced(BOTAN_INCLUDE_DIR BOTAN_LIBRARY BOTAN_LIBRARY_DEBUG)
