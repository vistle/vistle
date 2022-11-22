set(python_SOURCES)
set(python_HEADERS export.h)

if(Python_FOUND)
    set(python_SOURCES ${python_SOURCES} pythoninterface.cpp pythonstateaccessor.cpp)
    set(python_HEADERS ${python_HEADERS} pythoninterface.h pythonstateaccessor.h)
endif()

vistle_add_library(vistle_python EXPORT ${VISTLE_LIB_TYPE} ${python_SOURCES} ${python_HEADERS})
target_link_libraries(
    vistle_python
    PRIVATE Boost::system
    PRIVATE vistle_core
    PRIVATE vistle_util
    PRIVATE Threads::Threads)
target_compile_definitions(vistle_python PRIVATE EMBED_PYTHON)

if(Python_FOUND)
    target_link_libraries(vistle_python PRIVATE Python::Python)

    set(pythonmodule_SOURCES pythonmodule.cpp)
    set(pythonmodule_HEADERS module_export.h pythonmodule.h)

    vistle_add_library(vistle_pythonmodule EXPORT ${VISTLE_LIB_TYPE} ${pythonmodule_SOURCES} ${pythonmodule_HEADERS})
    target_link_libraries(
        vistle_pythonmodule
        PRIVATE Boost::system
        PRIVATE vistle_core
        PRIVATE vistle_util
        PRIVATE vistle_python
        PRIVATE Threads::Threads
        PRIVATE Python::Python)
    target_compile_definitions(vistle_pythonmodule PRIVATE EMBED_PYTHON)

    pybind11_add_module(_vistle NO_EXTRAS ${python_SOURCES} ${python_HEADERS} ${pythonmodule_SOURCES} ${pythonmodule_HEADERS})
    target_compile_definitions(_vistle PRIVATE vistle_python_EXPORTS)
    target_link_libraries(
        _vistle
        PRIVATE Boost::system
        PRIVATE Python::Python
        PRIVATE vistle_core
        PRIVATE vistle_util
        PRIVATE vistle_userinterface)

    add_custom_target(
        vistle_python_py ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vistle.py ${PROJECT_BINARY_DIR}/share/vistle/vistle.py
        DEPENDS vistle.py
        SOURCES vistle.py)

    install(FILES vistle.py DESTINATION share/vistle)
endif()
