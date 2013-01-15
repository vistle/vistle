find_package(PythonLibs)
find_package(Boost 1.40 REQUIRED COMPONENTS python thread system)

add_library(vistle_control SHARED 
   portmanager.cpp
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
        vistle_core
)
