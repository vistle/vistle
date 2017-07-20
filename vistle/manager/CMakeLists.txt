vistle_find_package(Boost 1.53 REQUIRED COMPONENTS system)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

if (VISTLE_SHM_PER_RANK)
   add_definitions(-DSHMPERRANK)
endif()

vistle_add_executable(vistle_manager
   vistle_manager.cpp
   clustermanager.h
   clustermanager.cpp
   datamanager.h
   datamanager.cpp
   communicator.h
   communicator.cpp
   executor.h
   executor.cpp
   portmanager.h
   portmanager.cpp
   )
target_link_libraries(vistle_manager
        vistle_boost_mpi
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)
if(NOT VISTLE_MULTI_PROCESS)
   target_link_libraries(vistle_manager ${CMAKE_DL_LIBS} vistle_module)
   if (NOT VISTLE_MODULES_SHARED)
      use_openmp()
      #include(../module/all_modules.cmake)
      foreach(m ${ALL_MODULES})
         if (APPLE)
            target_link_libraries(vistle_manager -Wl,-force_load ${m})
         else()
            target_link_libraries(vistle_manager -Wl,--whole-archive ${m} -Wl,--no-whole-archive)
         endif()
         #target_link_libraries(vistle_manager $<TARGET_OBJECTS:${m}>)
      endforeach()
   endif()
endif()

vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)
