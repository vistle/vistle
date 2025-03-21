set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(read)
add_subdirectory(insitu)
add_subdirectory(filter)
add_subdirectory(map)
add_subdirectory(geometry)
add_subdirectory(general)
add_subdirectory(info)
add_subdirectory(develop)

if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/module/univiz" AND EXISTS ${PROJECT_SOURCE_DIR}/module/univiz/CMakeLists.txt)
    set(UNIVIZ_VISTLE TRUE)
    add_subdirectory(univiz)
endif()

# has to come last for static module registration in VistleManager plugin
add_subdirectory(render)
