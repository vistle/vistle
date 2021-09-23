#
# - Try to find LZ4 compression library
# This will define
# LZ4_FOUND
# LZ4_INCLUDE_DIRS
# LZ4_LIBRARIES
#

find_path(LZ4_INCLUDE_DIR lz4.h)

find_library(LZ4_LIBRARY_DEBUG NAMES lz4d liblz4_d)
find_library(LZ4_LIBRARY_RELEASE NAMES lz4 liblz4)

include(SelectLibraryConfigurations)
select_library_configurations(LZ4)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4 DEFAULT_MSG LZ4_LIBRARY LZ4_INCLUDE_DIR)

if(LZ4_FOUND)
    message(STATUS "Found LZ4: ${LZ4_LIBRARY}")
    set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
    set(LZ4_LIBRARIES ${LZ4_LIBRARIES})
endif()

mark_as_advanced(LZ4_INCLUDE_DIR LZ4_LIBRARY)
