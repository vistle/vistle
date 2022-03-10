set(userinterface_SOURCES userinterface.cpp vistleconnection.cpp)

set(userinterface_HEADERS export.h userinterface.h vistleconnection.h)

if(Python_FOUND)
    set(userinterface_SOURCES ${userinterface_SOURCES} pythoninterface.cpp pythonmodule.cpp)

    set(userinterface_HEADERS ${userinterface_HEADERS} pythoninterface.h pythonmodule.h)
endif()

vistle_add_library(vistle_userinterface EXPORT ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})
target_link_libraries(
    vistle_userinterface
    PRIVATE Boost::system
    PRIVATE vistle_core
    PRIVATE vistle_util)

target_link_libraries(vistle_userinterface PRIVATE Threads::Threads)

if(Python_FOUND)
    pybind11_add_module(_vistle NO_EXTRAS ${userinterface_SOURCES} ${userinterface_HEADERS})
    target_compile_definitions(vistle_userinterface PRIVATE EMBED_PYTHON)
    target_compile_definitions(_vistle PRIVATE vistle_userinterface_EXPORTS)
    target_link_libraries(
        _vistle
        PRIVATE
        PRIVATE Boost::system
        PRIVATE Python::Python
        PRIVATE vistle_core
        PRIVATE vistle_util)
    target_link_libraries(vistle_userinterface PRIVATE Python::Python)
endif()
