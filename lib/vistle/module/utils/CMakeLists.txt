set(module_util_SOURCES)
set(module_util_HEADERS geo.h ghost.h)

if(NOT CMAKE_BUILD_TYPE STREQUAL "")
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()
