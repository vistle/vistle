vistle_find_package(NETCDF)
if(NOT NETCDF_FOUND)
    return()
endif()

set(HEADERS ReadIagNetcdf.h)
set(SOURCES ReadIagNetcdf.cpp)

add_module(ReadIagNetcdf "Read IAG data in NetCDF format" ${SOURCES} ${HEADERS})
target_compile_definitions(ReadIagNetcdf PRIVATE USE_NETCDF)
if(NETCDF_PARALLEL)
    target_compile_definitions(ReadIagNetcdf PRIVATE NETCDF_PARALLEL)
endif()
target_link_libraries(ReadIagNetcdf ${NETCDF_LIBRARIES} vistle_netcdf)
target_include_directories(ReadIagNetcdf SYSTEM PRIVATE ${NETCDF_INCLUDE_DIRS})
