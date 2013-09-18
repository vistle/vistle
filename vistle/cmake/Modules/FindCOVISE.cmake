# - Find COVISE

function(covise_find_library module library)

   find_library(${module}_LIBRARY
       NAMES ${library}
       PATH_SUFFIXES lib
       PATHS
       $ENV{COVISEDIR}/$ENV{ARCHSUFFIX}
       DOC "${module} - Library"
   )

endfunction()

if(NOT $ENV{ARCHSUFFIX} STREQUAL "")
   set(COVISE_ARCHSUFFIX $ENV{ARCHSUFFIX})
endif()

if(NOT $ENV{COVISEDIR} STREQUAL "")
   set(COVISEDIR $ENV{COVISEDIR})
endif()

if(COVISE_INCLUDE_DIR)
   set(COVISE_FIND_QUIETLY TRUE)
endif()

find_path(COVISE_INCLUDE_DIR "config/CoviseConfig.h"
   PATHS
   $ENV{COVISEDIR}/src/kernel
   DOC "COVISE - Headers"
)

covise_find_library(COVISE_UTIL coUtil)
covise_find_library(COVISE_FILE coFile)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(COVISE DEFAULT_MSG
   COVISE_ARCHSUFFIX COVISEDIR
   COVISE_UTIL_LIBRARY COVISE_FILE_LIBRARY
   COVISE_INCLUDE_DIR)
mark_as_advanced(COVISE_UTLI_LIBRARY COVISE_FILE_LIBRARY COVISE_INCLUDE_DIR)

if(COVISE_FOUND)
   set(COVISE_LIBRARIES ${COVISE_UTIL_LIBRARY} ${COVISE_FILE_LIBRARY})
   set(COVISE_INCLUDE_DIRS ${COVISE_INCLUDE_DIR})
endif()
