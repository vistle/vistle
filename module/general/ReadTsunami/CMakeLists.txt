# vistle_find_package(NETCDF)
# if(NOT NETCDF_FOUND)
#     return()
# endif()
vistle_find_package(PNETCDF)
if(NOT PNETCDF_FOUND)
    return()
endif()

set(HEADERS ReadTsunami.h)

set(SOURCES ReadTsunami.cpp)

add_module(ReadTsunami "read ChEESE Tsunami files" ${SOURCES} ${HEADERS})

# target_include_directories(ReadTsunami SYSTEM PRIVATE ${NETCDF_INCLUDE_DIRS})
target_include_directories(ReadTsunami SYSTEM PRIVATE ${PNETCDF_INCLUDE_DIRS})
# target_link_libraries(ReadTsunami vistle_module_util ${NETCDF_LIBRARIES})
target_link_libraries(ReadTsunami vistle_module_util ${PNETCDF_LIBRARIES})
