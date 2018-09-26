if(NOT PYTHONLIBS_FOUND)
   return()
endif()


set(EXTRA_LIBRARIES)
#vistle_find_package(Readline)
#if(READLINE_FOUND)
#   add_definitions(-DHAVE_READLINE)
#   include_directories(SYSTEM ${READLINE_INCLUDE_DIR})
#   set(EXTRA_LIBRARIES ${READLINE_LIBRARY})
#endif()


include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_PATH}
)
include_directories(
        ..
)

vistle_add_executable(blower blower.cpp)
target_link_libraries(blower
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${EXTRA_LIBRARIES}
        vistle_core
        vistle_userinterface
)

if(UNIX)
	target_link_libraries(blower ${CMAKE_DL_LIBS})
endif()
