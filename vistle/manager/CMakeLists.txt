include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

vistle_add_executable(vistle_manager
   vistle_manager.cpp
   clustermanager.h
   clustermanager.cpp
   communicator.cpp
   communicator.h
   executor.cpp
   executor.h
   portmanager.cpp
   portmanager.h
   )
target_link_libraries(vistle_manager
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
)

vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
)
