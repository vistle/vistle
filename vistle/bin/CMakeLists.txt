add_custom_target(vistle_spawner ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/spawn_vistle.sh ${EXECUTABLE_OUTPUT_PATH}/spawn_vistle.sh
   DEPENDS spawn_vistle.sh
   SOURCES spawn_vistle.sh
)

install(PROGRAMS spawn_vistle.sh DESTINATION bin)
