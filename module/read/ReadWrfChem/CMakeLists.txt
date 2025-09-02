vistle_find_package(NETCDF_CXX)
if(NOT NETCDF_CXX_FOUND)
    return()
endif()

set(HEADERS ReadWRFChem.h)

set(SOURCES ReadWRFChem.cpp)

add_module(ReadWrfChem "read WRF-Chem (Weather Research and Forecasting model coupled to Chemistry) data files" ${SOURCES} ${HEADERS})

target_link_libraries(ReadWrfChem PRIVATE ${NETCDF_CXX_LIBRARIES})
target_include_directories(ReadWrfChem SYSTEM PRIVATE ${NETCDF_CXX_INCLUDE_DIRS})
