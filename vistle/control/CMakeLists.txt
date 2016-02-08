add_custom_target(vistle_python ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
   ${CMAKE_CURRENT_SOURCE_DIR}/vistle.py
   ${PROJECT_BINARY_DIR}/share/vistle/vistle.py
   DEPENDS vistle.py
   SOURCES vistle.py
)

install(FILES vistle.py DESTINATION share/vistle)
