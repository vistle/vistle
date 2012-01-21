find_package(MPI REQUIRED)
find_package(Boost 1.40 REQUIRED COMPONENTS date_time)

add_library(vistle_core SHARED object.cpp message.cpp messagequeue.cpp module.cpp renderer.cpp parameter.cpp)

include_directories(
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
)
