set(control_SOURCES
    fileinfocrawler.cpp
    hub.cpp
    scanmodules.cpp
    scanscripts.cpp
    uiclient.cpp
    uimanager.cpp
    vistleurl.cpp)

set(control_HEADERS
    export.h
    fileinfocrawler.h
    hub.h
    scanmodules.h
    scanscripts.h
    uiclient.h
    uimanager.h
    vistleurl.h)

if(Python_FOUND)
    set(control_SOURCES ${control_SOURCES} pythoninterpreter.cpp)
    set(control_HEADERS ${control_HEADERS} pythoninterpreter.h)
endif()

vistle_add_library(vistle_control EXPORT ${VISTLE_LIB_TYPE} ${control_SOURCES} ${control_HEADERS})
target_link_libraries(
    vistle_control
    PRIVATE Boost::system
    PRIVATE Boost::program_options
    PRIVATE vistle_util
    PRIVATE vistle_core
    PRIVATE vistle_net
    PRIVATE vistle_libsim_connect
    PRIVATE Threads::Threads)
if(UNIX)
    target_link_libraries(vistle_control PRIVATE ${CMAKE_DL_LIBS})
endif()
target_link_libraries(vistle_control PRIVATE vistle_config)

if(Python_FOUND)
    target_compile_definitions(vistle_control PRIVATE HAVE_PYTHON)
    target_link_libraries(vistle_control PRIVATE Python::Python)
    target_link_libraries(vistle_control PRIVATE vistle_python)
    target_link_libraries(vistle_control PRIVATE vistle_pythonmodule)
endif()

if(VISTLE_USE_MPI)
    target_compile_definitions(vistle_control PRIVATE VISTLE_USE_MPI)
endif()
