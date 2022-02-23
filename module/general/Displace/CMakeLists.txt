set(HEADERS ../MapDrape/MapDrape.h)

set(SOURCES ../MapDrape/MapDrape.cpp)

add_module(Displace "move vertices according to mapped data" ${SOURCES} ${HEADERS})
target_compile_definitions(Displace PRIVATE DISPLACE)
target_link_libraries(Displace)
