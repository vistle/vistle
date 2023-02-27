#sets TetGen_INCLUDE_DIR and TetGen_LIBRARY if TetGen_FOUND

set(TetGen_HINTS $ENV{EXTERNLIBS}/TetGen $ENV{EXTERNLIBS}/TetGen/lib $ENV{EXTERNLIBS}/TetGen/include $ENV{EXTERNLIBS}/tetgen1.6.0/build
                 $ENV{EXTERNLIBS}/tetgen1.6.0/)

find_library(
    TetGen_LIBRARY tet
    PATHS ${TetGen_HINTS}
    PATH_SUFFIXES lib lib64)
message("TetGen_LIBRARY " ${TetGen_LIBRARY})
find_path(
    TetGen_INCLUDE_DIR
    NAMES tetgen.h
    PATHS ${TetGen_HINTS}
    PATH_SUFFIXES include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TetGen TetGen_LIBRARY TetGen_INCLUDE_DIR)
