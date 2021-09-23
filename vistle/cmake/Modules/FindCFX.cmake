# - Find CFX
# Find the CFX includes and library
#
#  CFX_INCLUDE_DIR - Where to find CFX includes
#  CFX_LIBRARIES   - List of libraries when using CFX
#  CFX_FOUND       - True if CFX was found

if(CFX_INCLUDE_DIR)
    set(CFX_FIND_QUIETLY TRUE)
endif(CFX_INCLUDE_DIR)

find_path(
    CFX_DIR
    NAMES "include/cfxExport.h"
    PATHS $ENV{EXTERNLIBS}/CFX-19
          $ENV{EXTERNLIBS}/CFX-181
          $ENV{EXTERNLIBS}/CFX-18
          $ENV{EXTERNLIBS}/CFX-17
          $ENV{EXTERNLIBS}/CFX-162
          $ENV{EXTERNLIBS}/CFX-15
          $ENV{EXTERNLIBS}/CFX-14
          $ENV{EXTERNLIBS}/CFX-12
    DOC "CFX installation directory")

find_path(
    CFX_INCLUDE_DIR
    NAMES "cfxExport.h"
    PATHS ${CFX_DIR}/include
    DOC "CFX - header")

find_library(
    CFX_IMPORT_LIBRARY
    NAMES "libmeshimport.a" "libmeshimport.lib"
    PATHS ${CFX_DIR}/lib)
find_library(
    CFX_EXPORT_LIBRARY
    NAMES "libmeshexport.a" "libmeshexport.lib"
    PATHS ${CFX_DIR}/lib)
find_library(
    CFX_PGT_LIBRARY
    NAMES "libpgtapi.a" "libpgtapi.lib"
    PATHS ${CFX_DIR}/lib)
find_library(
    CFX_UNITS_LIBRARY
    NAMES "libunits.a" "libunits.lib"
    PATHS ${CFX_DIR}/lib)
find_library(
    CFX_CCLAPILT_LIBRARY
    NAMES "libcclapilt.a" "libcclapilt.lib"
    PATHS ${CFX_DIR}/lib)
find_library(
    CFX_IO_LIBRARY
    NAMES "libio.a" "libio.lib"
    PATHS ${CFX_DIR}/lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    CFX
    DEFAULT_MSG
    CFX_DIR
    CFX_IMPORT_LIBRARY
    CFX_EXPORT_LIBRARY
    CFX_PGT_LIBRARY
    CFX_UNITS_LIBRARY
    CFX_CCLAPILT_LIBRARY
    CFX_IO_LIBRARY
    CFX_INCLUDE_DIR)
mark_as_advanced(CFX_LIBRARY CFX_INCLUDE_DIR)

if(CFX_FOUND)
    set(CFX_LIBRARIES ${CFX_IMPORT_LIBRARY} ${CFX_EXPORT_LIBRARY} ${CFX_PGT_LIBRARY} ${CFX_UNITS_LIBRARY} ${CFX_CCLAPILT_LIBRARY} ${CFX_IO_LIBRARY})
    set(CFX_INCLUDE_DIRS ${CFX_INCLUDE_DIR})
endif(CFX_FOUND)
