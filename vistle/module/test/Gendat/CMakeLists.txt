use_openmp()
enable_testing()

include_directories(SYSTEM
        ${Boost_INCLUDE_DIRS}
        ${MPI_C_INCLUDE_PATH}
)

add_module(Gendat Gendat.cpp)

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
#add_module(GendatChecker GendatChecker.cpp)
#target_link_libraries(GendatChecker
#        ${Boost_LIBRARIES}
#        ${MPI_C_LIBRARIES}
#        vistle_module
#)
