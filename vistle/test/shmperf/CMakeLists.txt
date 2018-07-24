if(NOT VISTLE_USE_SHARED_MEMORY)
   return()
endif()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
include_directories(
        ../..
)

add_executable(shmperf shmperf.cpp)
target_link_libraries(shmperf
        ${Boost_LIBRARIES}
        ${MPI_C_LIBRARIES}
        vistle_core
)
