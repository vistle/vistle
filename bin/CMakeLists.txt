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
        vistle_shell ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle_shell.sh ${EXECUTABLE_OUTPUT_PATH}/vistle_shell
        DEPENDS vistle_shell.sh
        SOURCES vistle_shell.sh)
endif()

install(PROGRAMS ${SPAWN_VISTLE} DESTINATION bin)
if(UNIX)
    install(
        PROGRAMS vistle_shell.sh
        RENAME vistle_shell
        DESTINATION bin)
endif()
