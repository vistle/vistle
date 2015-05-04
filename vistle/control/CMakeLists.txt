add_custom_target(vistle_python ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle.py ${LIBRARY_OUTPUT_PATH}/vistle.py
   DEPENDS vistle.py
   SOURCES vistle.py
)
