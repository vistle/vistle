vistle_find_package(PROJ4)
if (NOT PROJ4_FOUND)
  message("Skipping because of missing PROJ4")
  return()
else()
  if(NOT PROJ4_USED AND PROJ4_FOUND)
      set(PROJ4_USED TRUE)
      include_directories(SYSTEM ${PROJ4_INCLUDE_DIR})
      add_definitions(-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H)
      set(EXTRA_LIBS ${EXTRA_LIBS} ${PROJ4_LIBRARIES})
  endif()
endif()

set(HEADERS
  MapDrape.h
)

set(SOURCES
  MapDrape.cpp
)

add_module(MapDrape ${SOURCES} ${HEADERS})

target_link_libraries(MapDrape ${EXTRA_LIBS})

