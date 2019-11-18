vistle_find_package(XCB)
vistle_find_package(X11)



vistle_add_library(run_on_main_thread run_on_main_thread.h run_on_main_thread.cpp export.h)
target_link_libraries(run_on_main_thread
			vistle_util
)
include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ..
)

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()


set(SOURCES
   vistle_manager.cpp
)

vistle_add_executable(vistle_manager ${SOURCES})

target_link_libraries(vistle_manager
        vistle_manager_lib
		vistle_boost_mpi
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_control
        vistle_util
        vistle_core
        vistle_boost_mpi
)


vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_SYSTEM_LIBRARY}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)

add_subdirectory(managerLib)
