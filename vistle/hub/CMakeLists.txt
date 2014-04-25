find_package(Boost 1.40 REQUIRED OPTIONAL_COMPONENTS python)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)

set(hub_SOURCES
   hub.cpp
   uimanager.cpp
   uiclient.cpp
   )

if(PYTHONLIBS_FOUND)
   add_definitions(-DHAVE_PYTHON)
   add_definitions(-DVISTLE_CONTROL)
   set(hub_SOURCES ${hub_SOURCES} ../userinterface/pythoninterface.cpp ../userinterface/pythonmodule.cpp pythoninterpreter.cpp)
   set(hub_HEADERS ${hub_HEADERS} ../userinterface/pythoninterface.h ../userinterface/pythonmodule.h pythoninterpreter.h)
   include_directories(${PYTHON_INCLUDE_PATH})
   set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

add_executable(vistle ${hub_SOURCES})
target_link_libraries(vistle
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_control
)
