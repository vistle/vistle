# copied from https://notabug.org/lkundrak/wlc/src/setuid/CMake/FindXCB.cmake

# The MIT License (MIT)
#
# Copyright (c) 2014 Jari Vetoniemi
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

#  - Be sure to set the COMPONENTS to the components you want to link to
#  - The XCB_LIBRARIES variable is set ONLY to your COMPONENTS list
#  - To use only a specific component
#   check the XCB_LIBRARIES_${COMPONENT} variable

find_package(PkgConfig)
pkg_check_modules(PC_XCB QUIET xcb ${XCB_FIND_COMPONENTS})

find_library(XCB_LIBRARY xcb HINTS ${PC_XCB_LIBRARY_DIRS})
find_path(
    XCB_INCLUDE_DIRS xcb/xcb.h
    PATH_SUFFIXES xcb
    HINTS ${PC_XCB_INCLUDE_DIRS})

set(XCB_LIBRARIES ${XCB_LIBRARY})
foreach(COMPONENT ${XCB_FIND_COMPONENTS})
    find_library(XCB_LIBRARIES_${COMPONENT} xcb-${COMPONENT} HINTS ${PC_XCB_LIBRARY_DIRS})
    list(APPEND XCB_LIBRARIES ${XCB_LIBRARIES_${COMPONENT}})
    mark_as_advanced(XCB_LIBRARIES_${COMPONENT})
endforeach(COMPONENT ${XCB_FIND_COMPONENTS})

set(XCB_DEFINITIONS ${PC_XCB_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XCB DEFAULT_MSG XCB_LIBRARIES XCB_INCLUDE_DIRS)
mark_as_advanced(XCB_INCLUDE_DIRS XCB_LIBRARIES)
