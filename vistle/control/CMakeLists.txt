find_package(Boost 1.40 REQUIRED COMPONENTS thread system filesystem OPTIONAL_COMPONENTS python)
find_package(Readline)

set(EXTRA_LIBRARIES)
if(READLINE_FOUND)
   add_definitions(-DHAVE_READLINE)
   include_directories(${READLINE_INCLUDE_DIR})
   set(EXTRA_LIBRARIES ${READLINE_LIBRARY})
endif()

add_definitions(-DVISTLE_CONTROL)

set(control_SOURCES
   portmanager.cpp
   uiclient.cpp
   uimanager.cpp
   modulemanager.cpp
   communicator.cpp
   executor.cpp
)

set(control_HEADERS
   communicator.h
   executor.h
   export.h
   modulemanager.h
   portmanager.h
   uiclient.h
   uimanager.h
)

if(PYTHONLIBS_FOUND)
   add_definitions(-DHAVE_PYTHON)

   set(control_SOURCES ${control_SOURCES}
      ../userinterface/pythoninterface.cpp
      ../userinterface/pythonmodule.cpp
      client.cpp
      clientmanager.cpp
      )
   set(control_HEADERS ${control_HEADERS}
      ../userinterface/pythoninterface.h
      ../userinterface/pythonmodule.h
      client.h
      clientmanager.h
      )

   include_directories(${PYTHON_INCLUDE_PATH})
   set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
        ../userinterface
)

add_library(vistle_control ${VISTLE_LIB_TYPE} ${control_SOURCES} ${control_HEADERS})

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
