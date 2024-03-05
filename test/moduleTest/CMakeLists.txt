# set minimum cmake version
cmake_minimum_required(VERSION 3.14)

# project name
project(vistle_test LANGUAGES NONE)

# detect python
find_package(Python REQUIRED)

# enable testing functionality

enable_testing()

#TODO:
# 2. pass modulepath + refimgpath to test.py for each module with result img (to note: only for modules with result img)

# define tests
add_test(blub ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test.py)
