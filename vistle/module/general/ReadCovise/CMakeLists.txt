vistle_find_package(COVISE COMPONENTS File)
if(NOT COVISE_FILE_FOUND)
   return()
endif()

add_module(ReadCovise ReadCovise.cpp)

include_directories(
        ${COVISE_FILE_INCLUDE_DIRS}
)

target_link_libraries(ReadCovise
        ${COVISE_FILE_LIBRARY}
)
