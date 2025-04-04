set(HEADERS descriptions.h export.h)
set(SOURCES descriptions.cpp)
vistle_add_library(vistle_module_descriptions EXPORT ${VISTLE_LIB_TYPE} ${HEADERS} ${SOURCES})

get_filename_component(CATFILE ${CATEGORY_DESCRIPTION_FILE} NAME)
get_filename_component(MODFILE ${MODULE_DESCRIPTION_FILE} NAME)
target_compile_definitions(vistle_module_descriptions PRIVATE MODFILE="${MODFILE}" CATFILE="${CATFILE}")

if(VISTLE_MULTI_PROCESS OR VISTLE_MODULES_SHARED)
    target_link_libraries(vistle_module_descriptions PRIVATE vistle_util)
    set(MODDESCS)
else()
    set(MODDESCS ${MODULE_DESCRIPTION_FILE})
endif()

cmrc_add_resource_library(
    vistle_module_descriptions_resource
    NAMESPACE
    moduledescriptions
    WHENCE
    ${PROJECT_BINARY_DIR}/share/vistle
    ${CATEGORY_DESCRIPTION_FILE}
    ${MODDESCS})
vistle_export_library(vistle_module_descriptions_resource)
set_target_properties(vistle_module_descriptions_resource PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
if(APPLE)
    target_link_libraries(vistle_module_descriptions PRIVATE -Wl,-force_load vistle_module_descriptions_resource)
else()
    target_link_libraries(vistle_module_descriptions PRIVATE -Wl,--whole-archive vistle_module_descriptions_resource -Wl,--no-whole-archive)
endif()
