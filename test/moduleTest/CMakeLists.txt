# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name
project(vistle_test LANGUAGES NONE)

# detect python
find_package(Python REQUIRED)

# enable testing functionality

enable_testing()

#TODO:
# 1. generate module result images and compare them with the reference images
# 1.1 use generateCoverSnapshot to generate the images
# 2.

# define tests
add_test(blub ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py)
