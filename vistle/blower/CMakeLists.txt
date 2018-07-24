if(NOT PYTHONLIBS_FOUND)
   return()
endif()

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
        vistle_core
        vistle_userinterface
)

if(UNIX)
	target_link_libraries(blower ${CMAKE_DL_LIBS})
endif()
