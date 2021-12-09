if(WIN32)
    set(SPAWN_VISTLE spawn_vistle.bat)
else()
    set(SPAWN_VISTLE spawn_vistle.sh)
endif()

add_custom_target(
    vistle_spawner ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SPAWN_VISTLE} ${EXECUTABLE_OUTPUT_PATH}/${SPAWN_VISTLE}
    DEPENDS ${SPAWN_VISTLE}
    SOURCES ${SPAWN_VISTLE})

if(UNIX)
    add_custom_target(
        vistle_tui ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle_tui.sh ${EXECUTABLE_OUTPUT_PATH}/vistle_tui
        DEPENDS vistle_tui.sh
        SOURCES vistle_tui.sh)
endif()

if(UNIX)
    add_custom_target(
        vistle_opener ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle_opener.sh ${EXECUTABLE_OUTPUT_PATH}/vistle_opener
        DEPENDS vistle_opener.sh
        SOURCES vistle_opener.sh)
endif()

install(PROGRAMS ${SPAWN_VISTLE} DESTINATION bin)
if(UNIX)
    install(
        PROGRAMS vistle_tui.sh
        RENAME vistle_tui
        DESTINATION bin)
    install(
        PROGRAMS vistle_opener.sh
        RENAME vistle_opener
        DESTINATION bin)
endif()

configure_file(createDocumentation.sh.in createDocumentation.sh)
install(PROGRAMS createDocumentation.sh DESTINATION bin)
