#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_DIR
# GLEW_LIBRARY
#

if(WIN32)
    find_path(GLEW_INCLUDE_DIR GL/glew.h DOC "The directory where GL/glew.h resides")
    find_library(
        GLEW_LIBRARY
        NAMES glew GLEW glew32 glew32s
        PATHS
        DOC "The GLEW library")
else(WIN32)
    find_path(
        GLEW_INCLUDE_DIR
        GL/glew.h
        $ENV{EXTERNLIBS}/glew
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where GL/glew.h resides")
    find_library(
        GLEW_LIBRARY
        NAMES GLEW glew
        PATHS $ENV{EXTERNLIBS}/glew
              /usr/lib64
              /usr/lib
              /usr/local/lib64
              /usr/local/lib
              /sw/lib
              /opt/local/lib
        DOC "The GLEW library")
endif(WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)

if(GLEW_FOUND)
    set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
    set(GLEW_LIBRARIES ${GLEW_LIBRARY})
endif()
