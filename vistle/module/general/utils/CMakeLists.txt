vistle_find_package(Xdmf)
set(module_util_SOURCES str.cpp)

set(module_util_HEADERS c20.h geo.h str.h)

if(NOT CMAKE_BUILD_TYPE STREQUAL "")
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

vistle_add_library(vistle_module_util ${VISTLE_LIB_TYPE} ${module_util_SOURCES} ${module_util_HEADERS})
vistle_export_library(vistle_module_util ${VISTLE_LIB_TYPE} ${module_util_SOURCES} ${module_util_HEADERS})
target_include_directories(vistle_module_util SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_module_util)

if(NOT Xdmf_FOUND)
    return()
endif()

set(module_xdmf_SOURCES xdmf.cpp)
set(module_xdmf_HEADERS xdmf.h)

vistle_add_library(vistle_module_xdmf_util ${VISTLE_LIB_TYPE} ${module_xdmf_SOURCES} ${module_xdmf_HEADERS})
vistle_export_library(vistle_module_xdmf_util ${VISTLE_LIB_TYPE} ${module_xdmf_SOURCES} ${module_xdmf_HEADERS})
target_include_directories(vistle_module_xdmf_util SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
vistle_target_link_libraries(vistle_module_xdmf_util XdmfCore ${XDMF_LIBNAME})
