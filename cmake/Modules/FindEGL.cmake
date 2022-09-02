find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h)

find_library(
    EGL_LIBRARY
    NAMES EGL
    PATH_SUFFIXES lib64 lib lib32)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY EGL_INCLUDE_DIR)

if(EGL_FOUND)
    set(EGL_INCLUDE_DIRS ${EGL_INCLUDE_DIR})
    set(EGL_LIBRARIES ${EGL_LIBRARY})
endif()

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)
