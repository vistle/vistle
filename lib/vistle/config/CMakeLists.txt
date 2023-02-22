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
    ${COVCONFIG_HEADERS})
target_include_directories(vistle_config SYSTEM PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/3rdparty>)
target_compile_definitions(vistle_config PRIVATE CONFIG_NAMESPACE=vistle COVCONFIG_VISTLE)
target_include_directories(vistle_config PRIVATE ${COVCONFIG_PRIVATE_INCLUDES})
