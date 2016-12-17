vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system)
vistle_find_package(Boost 1.40 COMPONENTS system python)
vistle_find_package(Readline)

set(EXTRA_LIBRARIES)
if(READLINE_FOUND)
   add_definitions(-DHAVE_READLINE)
   include_directories(${READLINE_INCLUDE_DIR})
   set(EXTRA_LIBRARIES ${READLINE_LIBRARY})
endif()

set(userinterface_SOURCES
   userinterface.cpp
   vistleconnection.cpp
)

set(userinterface_HEADERS
   export.h
   userinterface.h
   vistleconnection.h
)

if (Boost_PYTHON_FOUND)
   include_directories(${PYTHON_INCLUDE_DIRS})
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

include_directories(
        ${Boost_INCLUDE_DIRS}
        ..
)

vistle_add_library(vistle_userinterface ${VISTLE_LIB_TYPE} ${userinterface_SOURCES} ${userinterface_HEADERS})

target_link_libraries(vistle_userinterface
        ${Boost_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_util
)
