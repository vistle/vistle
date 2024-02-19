vistle_find_package(NETCDF)
if(NOT NETCDF_C++_FOUND)
    return()
endif()

set(HEADERS ReadWRFChem.h)

set(SOURCES ReadWRFChem.cpp)

add_module(ReadWrfChem "read WRF-Chem (Weather Research and Forecasting model coupled to Chemistry) data files" ${SOURCES} ${HEADERS})

target_link_libraries(ReadWrfChem ${NETCDF_C++_LIBRARIES})
target_include_directories(ReadWrfChem SYSTEM PRIVATE ${NETCDF_C++_INCLUDE_DIRS})
