set(CONFIG_SOURCES config.cpp)
set(CONFIG_HEADERS array.h access.h file.h value.h config.h)

vistle_add_library(vistle_config EXPORT ${VISTLE_LIB_TYPE} ${CONFIG_SOURCES} ${CONFIG_HEADERS})
target_link_libraries(vistle_config vistle_covconfig)
target_include_directories(vistle_config SYSTEM PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/3rdparty>)
if(VISTLE_INSTALL_3RDPARTY)
    target_include_directories(vistle_config SYSTEM PUBLIC $<INSTALL_INTERFACE:include/vistle/3rdparty>)
endif()
