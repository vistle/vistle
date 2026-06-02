# - Find libraries for Extrae intstrumentation framework

# This module defines the following variables:
#   EXTRAE_FOUND  -  True if some libraries and include directory are found
# If set to TRUE, the following are also defined:
#   EXTRAE_INCLUDE_DIRS  -  The directory where to find the header file
#   EXTRAE_LIBRARIES  -  Where to find the library files
#
# This file is in the public domain

include(FindPackageHandleStandardArgs)

find_path(
    EXTRAE_INCLUDE_DIR
    NAMES extrae_user_events.h
    DOC "The Extrae include directory")
if(EXTRAE_INCLUDE_DIR)
    file(READ "${EXTRAE_INCLUDE_DIR}/extrae_version.h" ver)
    string(REGEX MATCH "EXTRAE_MAJOR ([0-9]*)" _ ${ver})
    set(EXTRAE_VERSION_MAJOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "EXTRAE_MINOR ([0-9]*)" _ ${ver})
    set(EXTRAE_VERSION_MINOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "EXTRAE_MICRO ([0-9]*)" _ ${ver})
    set(EXTRAE_VERSION_MICRO ${CMAKE_MATCH_1})
    set(EXTRAE_VERSION "${EXTRAE_VERSION_MAJOR}.${EXTRAE_VERSION_MINOR}.${EXTRAE_VERSION_MICRO}")
endif()

find_library(
    EXTRAE_MPITRACE_LIBRARY
    NAMES mpitrace
    DOC "The Extrae mpitrace library")

find_package_handle_standard_args(
    Extrae
    REQUIRED_VARS EXTRAE_MPITRACE_LIBRARY EXTRAE_INCLUDE_DIR
    VERSION_VAR EXTRAE_VERSION)

if(Extrae_FOUND)
    set(EXTRAE_INCLUDE_DIRS ${EXTRAE_INCLUDE_DIR})

    if(NOT EXTRAE_LIBRARIES)
        set(EXTRAE_LIBRARIES ${EXTRAE_MPITRACE_LIBRARY})
    endif()

    if(NOT TARGET Extrae::mpitrace)
        add_library(Extrae::mpitrace UNKNOWN IMPORTED)

        set_target_properties(Extrae::mpitrace PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EXTRAE_INCLUDE_DIRS}")

        set_property(
            TARGET Extrae::mpitrace
            APPEND
            PROPERTY IMPORTED_LOCATION "${EXTRAE_MPITRACE_LIBRARY}")
    endif()
endif()

mark_as_advanced(EXTRAE_INCLUDE_DIR EXTRAE_MPITRACE_LIBRARY)
