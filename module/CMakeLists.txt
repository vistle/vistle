add_subdirectory(test)
add_subdirectory(general)
add_subdirectory(render)

if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/vistle/module/univiz" AND EXISTS ${PROJECT_SOURCE_DIR}/vistle/module/univiz/CMakeLists.txt)
    set(UNIVIZ_VISTLE TRUE)
    add_subdirectory(univiz)
endif()
