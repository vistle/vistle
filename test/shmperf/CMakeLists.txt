if(NOT VISTLE_USE_SHARED_MEMORY AND NOT VISTLE_MULTI_PROCESS)
    return()
endif()

add_executable(vistle_shmperf shmperf.cpp)

target_include_directories(vistle_shmperf PRIVATE ../..)
target_link_libraries(
    vistle_shmperf
    PRIVATE Boost::boost
    PRIVATE Boost::serialization
    PRIVATE MPI::MPI_C
    PRIVATE vistle_util
    PRIVATE vistle_core
    PRIVATE Threads::Threads)
