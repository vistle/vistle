set(HEADERS descriptions.h export.h)
set(SOURCES descriptions.cpp)
vistle_add_library(vistle_module_descriptions EXPORT ${VISTLE_LIB_TYPE} ${HEADERS} ${SOURCES})

if(VISTLE_MODULES_SHARED)
    target_compile_definitions(vistle_module_descriptions PRIVATE -DVISTLE_MODULES_SHARED)
    target_link_libraries(vistle_module_descriptions PRIVATE vistle_util)
endif()

cmrc_add_resource_library(
    vistle_module_descriptions_resource
    NAMESPACE
    moduledescriptions
    WHENCE
    ${PROJECT_BINARY_DIR}/share/vistle
    ${CATEGORY_DESCRIPTION_FILE}
    ${MODULE_DESCRIPTION_FILE})
if(APPLE)
    target_link_libraries(vistle_module_descriptions PRIVATE -Wl,-force_load vistle_module_descriptions_resource)
else()
    target_link_libraries(vistle_module_descriptions PRIVATE -Wl,--whole-archive vistle_module_descriptions_resource -Wl,--no-whole-archive)
endif()
