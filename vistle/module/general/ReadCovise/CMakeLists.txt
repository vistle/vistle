include_directories(.)

add_definitions(-DNODLL)
add_library(vistle_covise_file STATIC file/covFiles.c)
if(NOT VISTLE_MULTI_PROCESS)
    set_target_properties(vistle_covise_file PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()

add_module(ReadCovise "read COVISE data" ReadCovise.cpp)
target_link_libraries(ReadCovise vistle_covise_file)

add_module(ReadCoviseDirectory "read COVISE data in a directory" ReadCovise.cpp)
target_compile_definitions(ReadCoviseDirectory PRIVATE READ_DIRECTORY)
target_link_libraries(ReadCoviseDirectory vistle_covise_file)
