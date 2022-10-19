vistle_find_package(PNETCDF)
if(NOT PNETCDF_FOUND)
    return()
endif()

set(HEADERS ReadTsunami.h)

set(SOURCES ReadTsunami.cpp)

#set(VISTLE_DOC_SKIP "no data")
add_module(ReadTsunami "read ChEESE Tsunami files" ${SOURCES} ${HEADERS})

target_include_directories(ReadTsunami SYSTEM PRIVATE ${PNETCDF_INCLUDE_DIRS})
target_link_libraries(ReadTsunami vistle_module ${PNETCDF_LIBRARIES})
