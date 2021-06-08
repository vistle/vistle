vistle_find_package(NETCDF)
if (NOT NETCDF_FOUND)
    return()
endif()

set(HEADERS
  ReadWRFChem.h
)

set(SOURCES
  ReadWRFChem.cpp
)

add_module(ReadWrfChem ${SOURCES} ${HEADERS})

target_link_libraries(ReadWrfChem ${NETCDF_LIBRARIES})
target_include_directories(ReadWrfChem SYSTEM PRIVATE ${NETCDF_INCLUDE_DIRS})
