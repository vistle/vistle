set(CONFIG_CMRC_NAMESPACE vistle)

# recursively pick up all .toml files in vistle/config and bake them into binary
file(
    GLOB_RECURSE CONFIG_FILES
    LIST_DIRECTORIES false
    ${PROJECT_SOURCE_DIR}/config/*.toml)
cmrc_add_resource_library(vistle_config_defaults NAMESPACE ${CONFIG_CMRC_NAMESPACE} WHENCE ${PROJECT_SOURCE_DIR}/config ${CONFIG_FILES})
set_target_properties(vistle_config_defaults PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
vistle_export_library(vistle_config_defaults)

set(CONFIG_SOURCES config.cpp)
set(CONFIG_HEADERS array.h access.h file.h value.h config.h)

set(PREFIX covconfig/)
include(${PREFIX}covconfig.cmake)

vistle_add_library(
    vistle_config
    EXPORT
    ${VISTLE_LIB_TYPE}
    ${CONFIG_SOURCES}
    ${CONFIG_HEADERS}
    ${COVCONFIG_SOURCES}
    ${COVCONFIG_HEADERS}
    ${COVCONFIG_DETAIL_HEADERS})
target_include_directories(vistle_config SYSTEM PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/3rdparty>)
target_compile_definitions(vistle_config PRIVATE CONFIG_NAMESPACE=vistle CONFIG_CMRC_NAMESPACE=${CONFIG_CMRC_NAMESPACE} COVCONFIG_VISTLE)
target_include_directories(vistle_config PRIVATE ${COVCONFIG_PRIVATE_INCLUDES})
target_link_libraries(vistle_config PRIVATE ${COVCONFIG_PRIVATE_LIBRARIES})
target_link_libraries(vistle_config PRIVATE vistle_config_defaults)
