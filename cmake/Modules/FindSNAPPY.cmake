# Find SNAPPY - A fast compressor/decompressor
#
# This module defines
#  SNAPPY_FOUND - whether the snappy library was found
#  SNAPPY_LIBRARIES - the snappy library
#  SNAPPY_INCLUDE_DIRS - the include path of the snappy library
#

if(SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

    # Already in cache
    set(SNAPPY_FOUND TRUE)

else(SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

    if(WIN32)
        find_path(
            SNAPPY_INCLUDE_DIR
            NAMES snappy.h
            PATHS $ENV{EXTERNLIBS}/snappy/include ${SNAPPY_ROOT_PATH}/include)
        find_library(
            SNAPPY_LIBRARY_DEBUG
            NAMES snappyd
            PATHS $ENV{EXTERNLIBS}/snappy/lib ${SNAPPY_ROOT_PATH}/Debug)
        find_library(
            SNAPPY_LIBRARY_RELEASE
            NAMES snappy
            PATHS $ENV{EXTERNLIBS}/snappy/lib ${SNAPPY_ROOT_PATH}/Release)
        if(MSVC_IDE)
            if(SNAPPY_LIBRARY_DEBUG AND SNAPPY_LIBRARY_RELEASE)
                set(SNAPPY_LIBRARY optimized ${SNAPPY_LIBRARY_RELEASE} debug ${SNAPPY_LIBRARY_DEBUG})
            else(SNAPPY_LIBRARY_DEBUG AND SNAPPY_LIBRARY_RELEASE)
                set(SNAPPY_LIBRARY NOTFOUND)
                message(STATUS "Could not find the debug AND release version of zlib")
            endif(SNAPPY_LIBRARY_DEBUG AND SNAPPY_LIBRARY_RELEASE)
        else(MSVC_IDE)
            string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
            if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
                set(SNAPPY_LIBRARY ${SNAPPY_LIBRARY_DEBUG})
            else(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
                set(SNAPPY_LIBRARY ${SNAPPY_LIBRARY_RELEASE})
            endif(CMAKE_BUILD_TYPE_TOLOWER MATCHES debug)
        endif(MSVC_IDE)
        mark_as_advanced(SNAPPY_LIBRARY_DEBUG SNAPPY_LIBRARY_RELEASE)
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(SNAPPY DEFAULT_MSG SNAPPY_LIBRARY SNAPPY_LIBRARY_DEBUG SNAPPY_INCLUDE_DIR)
    else(WIN32)
        find_library(
            SNAPPY_LIBRARY
            NAMES snappy
            PATHS)

        find_path(
            SNAPPY_INCLUDE_DIR
            NAMES snappy.h
            PATHS)

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(SNAPPY DEFAULT_MSG SNAPPY_LIBRARY SNAPPY_INCLUDE_DIR)

    endif(WIN32)
endif(SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

if(SNAPPY_FOUND)
    set(SNAPPY_INCLUDE_DIRS ${SNAPPY_INCLUDE_DIR})
    set(SNAPPY_LIBRARIES ${SNAPPY_LIBRARY})
endif()
