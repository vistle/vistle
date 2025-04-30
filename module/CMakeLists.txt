set(CMAKE_POSITION_INDEPENDENT_CODE ON)

vistle_category("Unspecified" "no category provided - please fix")

add_subdirectory(read)
add_subdirectory(insitu)
add_subdirectory(filter)
add_subdirectory(map)
add_subdirectory(geometry)
add_subdirectory(general)
add_subdirectory(write)
add_subdirectory(info)
add_subdirectory(develop)

set(UNIVIZ_DIR "${CMAKE_CURRENT_LIST_DIR}/univiz/modules/vistle")
if(IS_DIRECTORY "${UNIVIZ_DIR}" AND EXISTS "${UNIVIZ_DIR}/CMakeLists.txt")
    set(UNIVIZ_VISTLE TRUE)
    add_definitions(-DVISTLE)

    add_subdirectory(univiz/libs/vistle_ext)
    add_subdirectory(${UNIVIZ_DIR})
endif()

# has to come last for static module registration in VistleManager plugin
add_subdirectory(render)
