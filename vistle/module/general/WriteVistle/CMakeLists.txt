vistle_find_package(Boost 1.53 REQUIRED COMPONENTS serialization)

add_module(WriteVistle WriteVistle.cpp)

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)
