# Find zfp - Floating Point Compression
#
# This module defines
#  ZFP_FOUND - whether the zfp library was found
#  ZFP_LIBRARIES - the zfp library
#  ZFP_INCLUDE_DIRS - the include path of the zfp library
#

find_library(
    ZFP_LIBRARY
    NAMES zfp
    PATHS)

find_path(
    ZFP_INCLUDE_DIR
    NAMES zfp.h
    PATHS
    PATH_SUFFIXES include/zfp inc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZFP DEFAULT_MSG ZFP_LIBRARY ZFP_INCLUDE_DIR)

mark_as_advanced(ZFP_INCLUDE_DIR ZFP_LIBRARY)

if(ZFP_FOUND)
    set(ZFP_LIBRARIES ${ZFP_LIBRARY})
    set(ZFP_INCLUDE_DIRS ${ZFP_INCLUDE_DIR})
endif()
