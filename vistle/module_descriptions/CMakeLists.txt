include(../../3rdparty/cmrc/CMakeRC.cmake)

cmrc_add_resource_library(
    vistle_module_descriptions NAMESPACE moduledescriptions WHENCE ${PROJECT_BINARY_DIR}/share/vistle
    #${PROJECT_BINARY_DIR}/share/vistle/moduledescriptions.txt
    ${MODULE_DESCRIPTION_FILE})
