vistle_find_package(GDAL)
if(NOT GDAL_FOUND)
    return()
endif()

set(NAME IsoHeightSurface)
set(DESCRIPTION "extract surface at constant height")
add_definitions(-DISOHEIGHTSURFACE)

set(SOURCES MapHeight.cpp MapHeight.h)

include("../IsoSurface/IsoSurface.cmake")

target_link_libraries(${NAME} ${GDAL_LIBRARIES})
target_include_directories(${NAME} SYSTEM PRIVATE ${GDAL_INCLUDE_DIR})
