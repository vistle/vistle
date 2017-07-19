vistle_find_package(Boost 1.53 REQUIRED COMPONENTS program_options system)
vistle_find_package(Boost 1.53 COMPONENTS program_options system ${BOOST_PYTHON})

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)

set(hub_SOURCES
   hub.cpp
   uimanager.cpp
   uiclient.cpp
   )

if(Boost_PYTHON_FOUND OR Boost_PYTHON3_FOUND)
   include_directories(${PYTHON_INCLUDE_DIRS})
   add_definitions(-DHAVE_PYTHON)
   add_definitions(-DVISTLE_CONTROL)
   add_definitions(-Dvistle_userinterface_EXPORTS)
   set(hub_SOURCES ${hub_SOURCES} ../userinterface/pythoninterface.cpp ../userinterface/pythonmodule.cpp pythoninterpreter.cpp)
   set(hub_HEADERS ${hub_HEADERS} ../userinterface/pythoninterface.h ../userinterface/pythonmodule.h pythoninterpreter.h)
   set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

vistle_add_executable(vistle ${hub_SOURCES})
target_link_libraries(vistle
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_net
)

if(UNIX)
	target_link_libraries(vistle ${CMAKE_DL_LIBS})
endif()
