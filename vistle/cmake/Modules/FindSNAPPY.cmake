# Find SNAPPY - A fast compressor/decompressor
#
# This module defines
#  SNAPPY_FOUND - whether the snappy library was found
#  SNAPPY_LIBRARIES - the snappy library
#  SNAPPY_INCLUDE_DIRS - the include path of the snappy library
#

if (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

  # Already in cache
  set (SNAPPY_FOUND TRUE)

else (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

  find_library (SNAPPY_LIBRARY
    NAMES
    snappy
    PATHS
  )

  find_path (SNAPPY_INCLUDE_DIR
    NAMES
    snappy.h
    PATHS
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SNAPPY DEFAULT_MSG SNAPPY_LIBRARY SNAPPY_INCLUDE_DIR)

endif (SNAPPY_INCLUDE_DIR AND SNAPPY_LIBRARY)

if (SNAPPY_FOUND)
   set (SNAPPY_INCLUDE_DIRS ${SNAPPY_INCLUDE_DIR})
   set (SNAPPY_LIBRARIES ${SNAPPY_LIBRARY})
endif()
