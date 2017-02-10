if (WIN32)
   set(SPAWN_VISTLE spawn_vistle.bat)
else()
   set(SPAWN_VISTLE spawn_vistle.sh)
endif()

add_custom_target(vistle_spawner ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SPAWN_VISTLE} ${EXECUTABLE_OUTPUT_PATH}/${SPAWN_VISTLE}
   DEPENDS ${SPAWN_VISTLE}
   SOURCES ${SPAWN_VISTLE}
)

install(PROGRAMS ${SPAWN_VISTLE} DESTINATION bin)
