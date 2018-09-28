enable_testing()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_INCLUDE_PATH}
)

add_module(GenIsoDat GenIsoDat.cpp)
