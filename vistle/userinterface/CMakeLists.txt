set(userinterface_SOURCES
   userinterface.cpp
   vistleconnection.cpp
)

set(userinterface_HEADERS
   export.h
   userinterface.h
   vistleconnection.h
)

if (PYTHONLIBS_FOUND)
   include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
   set(userinterface_SOURCES
      ${userinterface_SOURCES}
      pythoninterface.cpp
      pythonmodule.cpp
      )

   set(userinterface_HEADERS
      ${userinterface_HEADERS}
      pythoninterface.h
      pythonmodule.h
      )
   set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

vistle_add_library(vistle_userinterface ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})
target_compile_definitions(vistle_userinterface PRIVATE -DEMBED_PYTHON)
target_link_libraries(vistle_userinterface
    PRIVATE ${Boost_SYSTEM_LIBRARY}
    PRIVATE ${EXTRA_LIBRARIES}
    PRIVATE vistle_core
    PRIVATE vistle_util
)

target_link_libraries(vistle_userinterface
    PRIVATE Threads::Threads
)

pybind11_add_module(_vistle NO_EXTRAS ${userinterface_SOURCES} ${userinterface_HEADERS})
target_compile_definitions(_vistle PRIVATE -Dvistle_userinterface_EXPORTS)
target_link_libraries(_vistle PRIVATE
    PRIVATE ${Boost_SYSTEM_LIBRARY}
    PRIVATE ${EXTRA_LIBRARIES}
    PRIVATE vistle_core
    PRIVATE vistle_util
)
