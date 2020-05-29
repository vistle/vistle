use_openmp()
enable_testing()

add_module(Gendat Gendat.cpp)

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
#add_module(GendatChecker GendatChecker.cpp)
#target_link_libraries(GendatChecker
#       Boost::boost
#        ${MPI_C_LIBRARIES}
#        vistle_module
#)
