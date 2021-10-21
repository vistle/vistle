#
# - Try to find Facebook zstd compression library
# This will define
# ZSTD_FOUND
# ZSTD_INCLUDE_DIRS
# ZSTD_LIBRARIES
#

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)

find_library(ZSTD_LIBRARY_DEBUG NAMES zstdd)
find_library(ZSTD_LIBRARY_RELEASE NAMES zstd)

include(SelectLibraryConfigurations)
select_library_configurations(ZSTD)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZSTD DEFAULT_MSG ZSTD_LIBRARY ZSTD_INCLUDE_DIR)

if(ZSTD_FOUND)
    message(STATUS "Found Zstd: ${ZSTD_LIBRARY}")
    set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
    set(ZSTD_LIBRARIES ${ZSTD_LIBRARIES})
endif()

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
