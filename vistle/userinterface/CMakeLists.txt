find_package(PythonLibs)
find_package(Boost 1.40 REQUIRED COMPONENTS python thread system)
find_package(Readline)

set(EXTRA_LIBRARIES)
if(READLINE_FOUND)
   add_definitions(-DHAVE_READLINE)
   include_directories(${READLINE_INCLUDE_DIR})
   set(EXTRA_LIBRARIES ${READLINE_LIBRARY})
endif()

set(userinterface_SOURCES
   userinterface.cpp
   pythonembed.cpp
   pythonmodule.cpp
   vistleconnection.cpp
)

set(userinterface_HEADERS
   export.h
   pythonembed.h
   pythonmodule.h
   userinterface.h
   vistleconnection.h
)

add_library(vistle_ui ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_PATH}
        ..
)

target_link_libraries(vistle_ui
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
