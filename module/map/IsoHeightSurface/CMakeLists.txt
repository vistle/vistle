vistle_find_package(GDAL)
if(NOT GDAL_FOUND)
    return()
endif()

set(NAME IsoHeightSurface)
set(DESCRIPTION "extract surface at constant height")

set(SOURCES MapHeight.cpp MapHeight.h)

include("../IsoSurface/IsoSurface.cmake")

target_link_libraries(${NAME} PRIVATE ${GDAL_LIBRARIES})
target_include_directories(${NAME} SYSTEM PRIVATE ${GDAL_INCLUDE_DIR})
target_compile_definitions(${NAME} PRIVATE ISOHEIGHTSURFACE)
