set(MODULE_CAN_CREATE_HASH True)
add_module(Gendat "generate scalar and vector test data on structured and unstructured grids" Gendat.cpp)
#generate_snapshots(Gendat Isosurface)
#enable_testing()

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
#add_module(GendatChecker GendatChecker.cpp)
#target_link_libraries(GendatChecker
#       Boost::boost
#        ${MPI_C_LIBRARIES}
#        vistle_module
#)
