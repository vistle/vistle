# Copyright (c) 2014 Andrew Kelley
# This file is MIT licensed.
# See http://opensource.org/licenses/MIT

# TINYXML2_FOUND
# TINYXML2_INCLUDE_DIR
# TINYXML2_LIBRARIES

find_path(TINYXML2_INCLUDE_DIR NAMES tinyxml2.h)
find_library(TINYXML2_LIBRARY NAMES tinyxml2)

include(FindPackageHandleStandardArgs)

if(MSVC)
    find_library(TINYXML2_LIBRARY_DEBUG NAMES tinyxml2d)
    find_package_handle_standard_args(TINYXML2 DEFAULT_MSG TINYXML2_LIBRARY TINYXML2_LIBRARY_DEBUG TINYXML2_INCLUDE_DIR)
else()
    find_package_handle_standard_args(TINYXML2 DEFAULT_MSG TINYXML2_LIBRARY TINYXML2_INCLUDE_DIR)
endif()

mark_as_advanced(TINYXML2_INCLUDE_DIR TINYXML2_LIBRARY TINYXML2_LIBRARY_DEBUG)

if(TINYXML2_FOUND)
    set(TINYXML2_INCLUDE_DIRS ${TINYXML2_INCLUDE_DIR})
    if(MSVC)
        set(TINYXML2_LIBRARIES optimized ${TINYXML2_LIBRARY} debug ${TINYXML2_LIBRARY_DEBUG})
    else()
        set(TINYXML2_LIBRARIES ${TINYXML2_LIBRARY})
    endif()
endif()
