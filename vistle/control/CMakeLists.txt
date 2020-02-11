set(control_SOURCES
   hub.cpp
   uimanager.cpp
   uiclient.cpp
   fileinfocrawler.cpp
)

set(control_HEADERS
   export.h
   hub.h
   uimanager.h
   uiclient.h
   fileinfocrawler.h
)

if(PYTHONLIBS_FOUND)
   include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
   add_definitions(-DHAVE_PYTHON)
   add_definitions(-DVISTLE_CONTROL)
   add_definitions(-Dvistle_userinterface_EXPORTS)
   add_definitions(-DEMBED_PYTHON)
   set(control_SOURCES ${control_SOURCES} ../userinterface/pythoninterface.cpp ../userinterface/pythonmodule.cpp pythoninterpreter.cpp)
   set(control_HEADERS ${control_HEADERS} ../userinterface/pythoninterface.h ../userinterface/pythonmodule.h pythoninterpreter.h)
   set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

vistle_add_library(vistle_control ${VISTLE_LIB_TYPE} ${control_SOURCES} ${control_HEADERS})
target_link_libraries(vistle_control
    PRIVATE ${Boost_SYSTEM_LIBRARY}
    PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY}
    PRIVATE ${EXTRA_LIBRARIES}
    PRIVATE vistle_util
    PRIVATE vistle_core
    PRIVATE vistle_net
    PRIVATE vistle_libsim_connect
)
target_link_libraries(vistle_control
    PRIVATE Threads::Threads
)
if(UNIX)
    target_link_libraries(vistle_control PRIVATE ${CMAKE_DL_LIBS})
endif()

add_custom_target(vistle_python ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
   ${CMAKE_CURRENT_SOURCE_DIR}/vistle.py
   ${PROJECT_BINARY_DIR}/share/vistle/vistle.py
   DEPENDS vistle.py
   SOURCES vistle.py
)

install(FILES vistle.py DESTINATION share/vistle)
