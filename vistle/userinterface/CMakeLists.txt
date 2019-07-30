vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system)
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

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
)
include_directories(
        ..
)

vistle_add_library(vistle_userinterface ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})
target_compile_definitions(vistle_userinterface PRIVATE -DEMBED_PYTHON)
target_link_libraries(vistle_userinterface
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)

pybind11_add_module(_vistle NO_EXTRAS ${userinterface_SOURCES} ${userinterface_HEADERS})
target_link_libraries(_vistle PRIVATE
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
