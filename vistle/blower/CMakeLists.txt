if(NOT PYTHONLIBS_FOUND)
   return()
endif()

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_PATH}
        ../core
        ..
)

vistle_add_executable(blower blower.cpp)
target_link_libraries(blower
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        vistle_core
        vistle_userinterface
)

if(UNIX AND NOT APPLE)
	target_link_libraries(blower
		-ldl
		-lutil
	)
endif()
