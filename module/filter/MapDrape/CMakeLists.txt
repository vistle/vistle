vistle_find_package(PROJ)
if(NOT PROJ_FOUND)
    return()
endif()

include_directories(SYSTEM ${PROJ_INCLUDE_DIRS})
if(PROJ_API)
    add_definitions(USE_PROJ_API)
    add_definitions(ACCEPT_USE_OF_DEPRECATED_PROJ_API_H)
endif()

set(HEADERS MapDrape.h)

set(SOURCES MapDrape.cpp)

add_module(MapDrape "map coordinates and drape to height field" ${SOURCES} ${HEADERS})
target_compile_definitions(MapDrape PRIVATE MAPDRAPE)
target_link_libraries(MapDrape ${PROJ_LIBRARIES})
