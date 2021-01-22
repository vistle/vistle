vistle_find_package(NetCDF)
if ((NOT NETCDF_FOUND))
  message("Skipping because of missing NETCDF")
  return()
else()
  include_directories(SYSTEM ${NETCDF_INCLUDES})
  set(EXTRA_LIBS ${EXTRA_LIBS} ${NETCDF_LIBRARIES} ${NETCDF_C++_LIBRARY})
endif()

set(HEADERS
    ReadTsunami.h
)

set(SOURCES
    ReadTsunami.cpp
)

add_module(ReadTsunami ${SOURCES} ${HEADERS})

target_link_libraries(ReadTsunami ${EXTRA_LIBS})

