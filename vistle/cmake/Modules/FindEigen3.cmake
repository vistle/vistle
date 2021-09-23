# Find Eigen 3 - A C++ Template Library for Linear Algebra
#
# This module defines
#  EIGEN3_FOUND - whether the Eigen 3 library was found
#  EIGEN3_INCLUDE_DIR - the include path of the Eigen 3 library
#

if(NOT EIGEN3_INCLUDE_DIR)
    find_path(
        EIGEN3_INCLUDE_DIR
        NAMES eigen3/signature_of_eigen3_matrix_library
        PATHS)
endif(NOT EIGEN3_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EIGEN3 DEFAULT_MSG EIGEN3_INCLUDE_DIR)

if(EIGEN3_FOUND)
    set(EIGEN3_INCLUDE_PATH ${EIGEN3_INCLUDE_DIR})
endif(EIGEN3_FOUND)
