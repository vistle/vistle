cmake_minimum_required(VERSION 3.5)
project(teciosrc)

file(GLOB TECIOSRC_SOURCES teciosrc/*.cpp teciosrc/*.c)
file(GLOB TECIOSRC_HEADERS teciosrc/*.h)

add_library(teciosrc STATIC ${TECIOSRC_SOURCES} ${TECIOSRC_HEADERS})

target_include_directories(teciosrc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/teciosrc>
    $<INSTALL_INTERFACE:teciosrc>
)

set_target_properties(teciosrc PROPERTIES
    OUTPUT_NAME "teciosrc"
    VERSION 1.0
    SOVERSION 1
)