vistle_find_package(NetCDF)
if ((NOT NETCDF_FOUND))
  message("Skipping because of missing NETCDF")
  return()
else()
  include_directories(SYSTEM ${NETCDF_INCLUDES})
  set(EXTRA_LIBS ${EXTRA_LIBS} ${NETCDF_LIBRARIES} ${NETCDF_C++_LIBRARY})
endif()

set(HEADERS
  ReadWRFChem.h
)

set(SOURCES
  ReadWRFChem.cpp
)

add_module(ReadWRFChem ${SOURCES} ${HEADERS})

target_link_libraries(ReadWRFChem ${EXTRA_LIBS})

