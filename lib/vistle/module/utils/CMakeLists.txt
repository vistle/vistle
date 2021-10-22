set(module_util_SOURCES)
set(module_util_HEADERS geo.h ghost.h)

if(NOT CMAKE_BUILD_TYPE STREQUAL "")
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
endif()

#vistle_add_library(vistle_module_util ${VISTLE_LIB_TYPE} ${module_util_SOURCES} ${module_util_HEADERS})
#vistle_export_library(vistle_module_util ${VISTLE_LIB_TYPE} ${module_util_SOURCES} ${module_util_HEADERS})
#target_include_directories(vistle_module_util SYSTEM PRIVATE ${XDMF_XdmfCore_INCLUDE_DIRS} ${XDMF_INCLUDE_DIRS})
#vistle_target_link_libraries(vistle_module_util)
