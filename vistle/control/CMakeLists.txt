find_package(PythonLibs)
find_package(Boost 1.40 REQUIRED COMPONENTS python thread system)
find_package(Readline)

if(READLINE_FOUND)
   add_definitions(-DHAVE_READLINE)
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
        ${READLINE_INCLUDE_DIR}
        ..
)

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${READLINE_LIBRARY}
        vistle_core
)
