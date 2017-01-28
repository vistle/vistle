vistle_find_package(Boost 1.40 REQUIRED COMPONENTS system)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
        ..
)

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
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)

vistle_add_executable(clean_vistle clean_vistle.cpp)
target_link_libraries(clean_vistle
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
        vistle_boost_mpi
)
