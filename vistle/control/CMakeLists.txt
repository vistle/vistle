find_package(Boost 1.40 REQUIRED COMPONENTS thread system filesystem)
find_package(Readline)

add_definitions(-DVISTLE_CONTROL)

set(control_SOURCES
   portmanager.cpp
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
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

add_library(vistle_control ${VISTLE_LIB_TYPE} ${control_SOURCES} ${control_HEADERS})

target_link_libraries(vistle_control
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_util
)

add_custom_target(vistle_python ALL
   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle.py ${LIBRARY_OUTPUT_PATH}/vistle.py
   DEPENDS vistle.py
   SOURCES vistle.py
)
