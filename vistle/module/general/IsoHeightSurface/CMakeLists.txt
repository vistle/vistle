set(NAME IsoHeightSurface)
add_definitions(-DISOHEIGHTSURFACE)

vistle_find_package(GDAL)
if (NOT GDAL_FOUND)
  message("Skipping because of missing GDAL")
  return()
else()
  if(NOT GDAL_USED AND GDAL_FOUND)
      set(GDAL_USED TRUE)
      include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
      set(EXTRA_LIBS ${EXTRA_LIBS} ${GDAL_LIBRARIES})
  endif()
endif()

set(SOURCES
	MapHeight.cpp
	MapHeight.h
)

include("../IsoSurface/IsoSurface.cmake")

target_link_libraries(${NAME} ${EXTRA_LIBS})
