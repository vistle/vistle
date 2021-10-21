#
## Copyright 2003 Sandia Coporation
## Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
## license for use of this work by or on behalf of the U.S. Government.
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that this Notice and any statement
## of authorship are reproduced on all copies.
#
# Id
#
# Find an IceT installation or build tree.
#
# The following variables are set if IceT is found.  If IceT is not found,
# ICET_FOUND is set to false.
#
# ICET_FOUND            - Set to true when IceT is found.
# ICET_USE_FILE         - CMake source file to setup a project to use IceT
# ICET_CORE_LIBS        - List of core IceT libraries that all IceT apps need.
# ICET_MPI_LIBS         - List of libraries for using IceT with MPI (not
#                         including MPI itself).
# ICET_MAJOR_VERSION    - The IceT major version number.
# ICET_MINOR_VERSION    - The IceT minor version number.
# ICET_PATCH_VERSION    - The IceT patch version number.
#
# The following variables are also set, but their use is handled for you
# when including ICET_USE_FILE.
#
# ICET_INCLUDE_DIRS     - Include directories for IceT headers.
# ICET_LIBRARY_DIRS     - Link directories for IceT libraries.
#
# The following cache entries must be set by the user to locate IceT:
#
# ICET_DIR              - The directory containing ICETConfig.cmake.
#

set(ICET_DIR_DESCRIPTION "directory containing IceTConfig.cmake.  This is either the root of the build tree, or PREFIX/lib for an installation.")
set(ICET_DIR_MESSAGE "IceT not found.  Set ICET_DIR to the ${ICET_DIR_DESCRIPTION}")

if(NOT ICET_DIR)
    # Get the system search path as a list.
    if(UNIX)
        string(REGEX MATCHALL "[^:]+" ICET_DIR_SEARCH1 "$ENV{PATH}")
    else(UNIX)
        string(REGEX REPLACE "\\\\" "/" ICET_DIR_SEARCH1 "$ENV{PATH}")
    endif(UNIX)
    string(REGEX REPLACE "/;" ";" ICET_DIR_SEARCH2 "${ICET_DIR_SEARCH1}")

    # Construct a set of paths relative to the system search path.
    set(ICET_DIR_SEARCH "")
    foreach(dir ${ICET_DIR_SEARCH2})
        set(ICET_DIR_SEARCH ${ICET_DIR_SEARCH} "${dir}/../lib")
    endforeach(dir)

    find_path(
        ICET_DIR
        IceTConfig.cmake
        # Look in places relative to the system executable search path.
        ${ICET_DIR_SEARCH}
        # Look in standard UNIX install locations.
        /usr/local/lib
        /usr/lib
        # Look in standard Win32 install locations.
        "C:/Program Files/IceT/lib"
        # Give documentation to user in case we can't find it.
        DOC "The ${ICET_DIR_DESCRIPTION}")
endif(NOT ICET_DIR)

# If IceT was found, load the configuration file to get the rest of the
# settings.
if(ICET_DIR)
    # Make sure the ICETConfig.cmake file exists in the directory provided.
    if(EXISTS ${ICET_DIR}/IceTConfig.cmake)

        # We found IceT.  Load the settings.
        set(ICET_FOUND 1)
        include(${ICET_DIR}/IceTConfig.cmake)

    else(EXISTS ${ICET_DIR}/IceTConfig.cmake)
        # We did not find IceT.
        set(ICET_FOUND 0)
    endif(EXISTS ${ICET_DIR}/IceTConfig.cmake)
else(ICET_DIR)
    # We did not find IceT.
    set(ICET_FOUND 0)
endif(ICET_DIR)

#-----------------------------------------------------------------------------
if(NOT ICET_FOUND)
    # IceT not found, explain to the user how to specify its location.
    if(NOT ICET_FIND_QUIETLY)
        message(${ICET_DIR_MESSAGE})
    endif(NOT ICET_FIND_QUIETLY)
endif(NOT ICET_FOUND)
