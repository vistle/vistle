find_package(PythonLibs)
find_package(Boost 1.40 REQUIRED COMPONENTS python thread system)
find_package(Readline)

set(EXTRA_LIBRARIES)
if(READLINE_FOUND)
   add_definitions(-DHAVE_READLINE)
   include_directories(${READLINE_INCLUDE_DIR})
   set(EXTRA_LIBRARIES ${READLINE_LIBRARY})
endif()

add_library(vistle_control SHARED 
   portmanager.cpp
   client.cpp
   clientmanager.cpp
   communicator.cpp
   executor.cpp
   pythonembed.cpp
   pythonmodule.cpp
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
        ${PYTHON_INCLUDE_PATH}
        ..
)

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
