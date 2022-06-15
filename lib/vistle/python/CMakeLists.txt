set(python_SOURCES)
set(python_HEADERS export.h)

if(Python_FOUND)
    set(python_SOURCES ${python_SOURCES} pythoninterface.cpp pythonmodule.cpp)
    set(python_HEADERS ${python_HEADERS} pythoninterface.h pythonmodule.h)
endif()

vistle_add_library(vistle_python EXPORT ${VISTLE_LIB_TYPE} ${python_SOURCES} ${python_HEADERS})
target_link_libraries(
    vistle_python
    PRIVATE Boost::system
    PRIVATE vistle_core
    PRIVATE vistle_util)

target_link_libraries(vistle_python PRIVATE Threads::Threads)

if(Python_FOUND)
    target_compile_definitions(vistle_python PRIVATE EMBED_PYTHON)
    target_link_libraries(vistle_python PRIVATE Python::Python)

    pybind11_add_module(_vistle NO_EXTRAS ${python_SOURCES} ${python_HEADERS})
    target_compile_definitions(_vistle PRIVATE vistle_python_EXPORTS)
    target_link_libraries(
        _vistle
        PRIVATE Boost::system
        PRIVATE Python::Python
        PRIVATE vistle_core
        PRIVATE vistle_util)
endif()
