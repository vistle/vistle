find_package(PythonLibs)
find_package(Boost 1.40 REQUIRED COMPONENTS python thread system filesystem)
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
   client.cpp
   clientmanager.cpp
   modulemanager.cpp
   communicator.cpp
   executor.cpp
   ../userinterface/pythoninterface.cpp
   ../userinterface/pythonmodule.cpp
)

set(control_HEADERS
   client.h
   clientmanager.h
   communicator.h
   executor.h
   export.h
   modulemanager.h
   portmanager.h
   ../userinterface/pythoninterface.h
   ../userinterface/pythonmodule.h
   uiclient.h
   uimanager.h
)

add_library(vistle_control ${VISTLE_LIB_TYPE} ${control_SOURCES} ${control_HEADERS})

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PYTHON_INCLUDE_PATH}
        ..
        ../userinterface
)

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
