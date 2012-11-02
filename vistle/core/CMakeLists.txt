add_library(vistle_core SHARED object.cpp shm.cpp message.cpp messagequeue.cpp module.cpp renderer.cpp parameter.cpp vector.cpp tools.cpp)

if(UNIX AND NOT APPLE)
	target_link_libraries(vistle_core
		-lrt
	)
endif()

target_link_libraries(vistle_core
        ${MPI_LIBRARIES}
)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
)
